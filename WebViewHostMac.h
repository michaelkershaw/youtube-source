#ifndef WEBVIEWHOSTMAC_H
#define WEBVIEWHOSTMAC_H

#ifdef __APPLE__

#include <string>
#include <functional>
#include <atomic>

// Keep the same public API as the Windows WebViewHost so call sites are identical.
typedef void* HINSTANCE;

// Hosts an NSWindow containing a WKWebView.
// Show() must be called on a dedicated thread; it dispatches the window onto the
// main thread (AppKit requirement) and blocks until the window is closed.
// PostJson() is thread-safe.
class WebViewHost {
public:
    using MessageHandler = std::function<void(const std::string& type, const std::string& payloadJson)>;
    using Logger = std::function<void(const std::string&)>;

    WebViewHost() = default;
    ~WebViewHost();

    // WKWebView ships with macOS - always available.
    static bool IsRuntimeAvailable() { return true; }

    void SetLogger(Logger logger) { Log = std::move(logger); }
    void SetMessageHandler(MessageHandler handler) { OnMessage = std::move(handler); }

    // uiFolder: local folder containing index.html (loaded via loadFileURL)
    // Blocks until the window is closed.
    void Show(HINSTANCE hInstance, const std::wstring& title,
              const std::string& uiFolder, const std::string& userDataFolder,
              int width = 1100, int height = 720);

    // Thread-safe: post a JSON string to the web page (delivered via window.__hostMessage)
    void PostJson(const std::string& json);

    // Thread-safe: request the window to close
    void Close();

    bool IsVisible() const;
    void BringToFront();

    // Internal (called from the Objective-C side)
    void _HandleScriptMessage(const std::string& json);
    void _WindowClosed();

private:
    void LogMsg(const std::string& msg) const { if (Log) Log(msg); }

    void* Window = nullptr;      // NSWindow*
    void* WebView = nullptr;     // WKWebView*
    void* Delegate = nullptr;    // script message handler object

    MessageHandler OnMessage;
    Logger Log;
    std::atomic<bool> Ready{false};
    std::atomic<bool> Closed{false};
};

#endif // __APPLE__
#endif // WEBVIEWHOSTMAC_H
