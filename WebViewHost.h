#ifndef WEBVIEWHOST_H
#define WEBVIEWHOST_H

#include <windows.h>
#include <string>
#include <functional>
#include <atomic>
#include <wrl.h>
#include "WebView2.h"

// Hosts a frameless top-level window containing a WebView2 control.
// Show() must be called on a dedicated thread; it runs a message loop until
// the window is closed. PostJson() is thread-safe.
class WebViewHost {
public:
    // type + raw JSON payload string of the message from JS
    using MessageHandler = std::function<void(const std::string& type, const std::string& payloadJson)>;
    using Logger = std::function<void(const std::string&)>;

    WebViewHost() = default;
    ~WebViewHost();

    // True if a WebView2 runtime (Evergreen or fixed) is installed.
    static bool IsRuntimeAvailable();

    void SetLogger(Logger logger) { Log = std::move(logger); }
    void SetMessageHandler(MessageHandler handler) { OnMessage = std::move(handler); }

    // uiFolder: local folder containing index.html (mapped to https://app.youtube-source/)
    // Blocks running a message loop until the window is destroyed.
    void Show(HINSTANCE hInstance, const std::wstring& title,
              const std::string& uiFolder, const std::string& userDataFolder,
              int width = 1100, int height = 720);

    // Thread-safe: post a JSON string to the web page (received via window.chrome.webview events)
    void PostJson(const std::string& json);

    // Thread-safe: request the window to close
    void Close();

    bool IsVisible() const { return hWnd != NULL && IsWindowVisible(hWnd); }
    HWND GetHWND() const { return hWnd; }
    void BringToFront();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateWebView();
    void ResizeWebView();
    void HandleWebMessage(const std::wstring& json);
    void LogMsg(const std::string& msg) const { if (Log) Log(msg); }

    HWND hWnd = NULL;
    HINSTANCE hInst = NULL;
    std::wstring UiFolderW;
    std::wstring UserDataFolderW;

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> Controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> WebView;

    MessageHandler OnMessage;
    Logger Log;
    std::atomic<bool> Ready{false};

    static const wchar_t* WND_CLASS_NAME;
    static const UINT WM_POST_JSON = WM_APP + 10;   // lParam = new std::string*
    static const UINT WM_CLOSE_REQUEST = WM_APP + 11;
};

#endif
