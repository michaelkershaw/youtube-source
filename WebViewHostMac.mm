// WKWebView host for macOS - counterpart of the Windows WebViewHost (WebView2).
// Compile as Objective-C++ (.mm). Link: -framework Cocoa -framework WebKit

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "WebViewHostMac.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>

// ---------------------------------------------------------------------------
// Objective-C bridge object: receives script messages + window close events
// ---------------------------------------------------------------------------
@interface YTSWebViewBridge : NSObject <WKScriptMessageHandler, NSWindowDelegate>
@property (nonatomic, assign) WebViewHost* host;
@end

@implementation YTSWebViewBridge

- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)message
{
    if (!self.host) return;
    if ([message.body isKindOfClass:[NSString class]]) {
        self.host->_HandleScriptMessage(std::string([(NSString*)message.body UTF8String]));
    }
}

- (void)windowWillClose:(NSNotification*)notification
{
    if (self.host) self.host->_WindowClosed();
}

@end

// ---------------------------------------------------------------------------
// WebViewHost implementation
// ---------------------------------------------------------------------------

WebViewHost::~WebViewHost()
{
    Close();
}

static std::string WideToUtf8(const std::wstring& w)
{
    std::string out;
    out.reserve(w.size());
    for (wchar_t c : w) {
        if (c < 0x80) out.push_back((char)c);
        else if (c < 0x800) {
            out.push_back((char)(0xC0 | (c >> 6)));
            out.push_back((char)(0x80 | (c & 0x3F)));
        } else {
            out.push_back((char)(0xE0 | (c >> 12)));
            out.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

void WebViewHost::Show(HINSTANCE /*hInstance*/, const std::wstring& title,
                       const std::string& uiFolder, const std::string& /*userDataFolder*/,
                       int width, int height)
{
    Closed = false;
    std::string titleUtf8 = WideToUtf8(title);

    // All AppKit work must happen on the main thread.
    dispatch_sync(dispatch_get_main_queue(), ^{
        @autoreleasepool {
            YTSWebViewBridge* bridge = [[YTSWebViewBridge alloc] init];
            bridge.host = this;
            Delegate = (__bridge_retained void*)bridge;

            WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
            [config.userContentController addScriptMessageHandler:bridge name:@"host"];

            NSRect frame = NSMakeRect(0, 0, width, height);
            WKWebView* webView = [[WKWebView alloc] initWithFrame:frame configuration:config];
            WebView = (__bridge_retained void*)webView;

            NSWindow* window = [[NSWindow alloc]
                initWithContentRect:frame
                          styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                     NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                            backing:NSBackingStoreBuffered
                              defer:NO];
            window.title = [NSString stringWithUTF8String:titleUtf8.c_str()];
            window.contentView = webView;
            window.delegate = bridge;
            window.releasedWhenClosed = NO;
            [window center];
            Window = (__bridge_retained void*)window;

            // Load index.html from the UI folder (grant read access to the folder)
            NSString* folder = [NSString stringWithUTF8String:uiFolder.c_str()];
            NSURL* folderUrl = [NSURL fileURLWithPath:folder isDirectory:YES];
            NSURL* indexUrl = [folderUrl URLByAppendingPathComponent:@"index.html"];
            [webView loadFileURL:indexUrl allowingReadAccessToURL:folderUrl];

            [window makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];
            Ready = true;
        }
    });

    LogMsg("WebViewHostMac: window shown, ui=" + uiFolder);

    // Block this (dedicated) thread until the window closes, mirroring the
    // Windows message-loop behaviour that callers rely on.
    while (!Closed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LogMsg("WebViewHostMac: window closed");
}

void WebViewHost::PostJson(const std::string& json)
{
    if (!Ready || Closed || !WebView) return;

    // Escape the JSON string as a JS string literal and call window.__hostMessage
    NSString* payload = [NSString stringWithUTF8String:json.c_str()];
    if (!payload) return;
    NSData* data = [NSJSONSerialization dataWithJSONObject:@[payload] options:0 error:nil];
    if (!data) return;
    NSString* arr = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    // arr is like ["<escaped json>"] - slice off the brackets to get the literal
    NSString* literal = [arr substringWithRange:NSMakeRange(1, arr.length - 2)];
    NSString* script = [NSString stringWithFormat:@"window.__hostMessage(%@);", literal];

    WKWebView* webView = (__bridge WKWebView*)WebView;
    dispatch_async(dispatch_get_main_queue(), ^{
        [webView evaluateJavaScript:script completionHandler:nil];
    });
}

void WebViewHost::Close()
{
    if (Closed || !Window) return;
    NSWindow* window = (__bridge NSWindow*)Window;
    dispatch_async(dispatch_get_main_queue(), ^{
        [window close];
    });
}

bool WebViewHost::IsVisible() const
{
    if (!Window || Closed) return false;
    NSWindow* window = (__bridge NSWindow*)Window;
    return window.isVisible;
}

void WebViewHost::BringToFront()
{
    if (!Window || Closed) return;
    NSWindow* window = (__bridge NSWindow*)Window;
    dispatch_async(dispatch_get_main_queue(), ^{
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    });
}

void WebViewHost::_HandleScriptMessage(const std::string& json)
{
    if (!OnMessage) return;
    // Extract "type" and "payload" from {"type":"...","payload":{...}}
    try {
        NSData* data = [NSData dataWithBytes:json.data() length:json.size()];
        NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
        if (![dict isKindOfClass:[NSDictionary class]]) return;
        NSString* type = dict[@"type"];
        if (![type isKindOfClass:[NSString class]]) return;
        id payload = dict[@"payload"];
        std::string payloadJson = "{}";
        if (payload) {
            NSData* pd = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
            if (pd) payloadJson = std::string((const char*)pd.bytes, pd.length);
        }
        OnMessage(std::string([type UTF8String]), payloadJson);
    } catch (...) {
        LogMsg("WebViewHostMac: failed to parse script message");
    }
}

void WebViewHost::_WindowClosed()
{
    Closed = true;
    Ready = false;
    if (Delegate) {
        YTSWebViewBridge* bridge = (__bridge_transfer YTSWebViewBridge*)Delegate;
        bridge.host = nullptr;
        Delegate = nullptr;
    }
    if (WebView) {
        WKWebView* webView = (__bridge_transfer WKWebView*)WebView;
        (void)webView;
        WebView = nullptr;
    }
    if (Window) {
        NSWindow* window = (__bridge_transfer NSWindow*)Window;
        (void)window;
        Window = nullptr;
    }
}

#endif // __APPLE__
