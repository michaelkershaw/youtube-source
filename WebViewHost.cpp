#include "WebViewHost.h"
#include "json.hpp"
#include <objbase.h>

using namespace Microsoft::WRL;
using json = nlohmann::json;

const wchar_t* WebViewHost::WND_CLASS_NAME = L"YouTubeSourceWebViewHost";

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

WebViewHost::~WebViewHost()
{
    Close();
}

bool WebViewHost::IsRuntimeAvailable()
{
    LPWSTR version = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
    if (SUCCEEDED(hr) && version) {
        CoTaskMemFree(version);
        return true;
    }
    return false;
}

void WebViewHost::BringToFront()
{
    if (hWnd) {
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
    }
}

void WebViewHost::PostJson(const std::string& jsonStr)
{
    if (!hWnd) return;
    auto* heapStr = new std::string(jsonStr);
    if (!PostMessage(hWnd, WM_POST_JSON, 0, (LPARAM)heapStr)) {
        delete heapStr;
    }
}

void WebViewHost::Close()
{
    if (hWnd) PostMessage(hWnd, WM_CLOSE_REQUEST, 0, 0);
}

void WebViewHost::Show(HINSTANCE hInstance, const std::wstring& title,
                       const std::string& uiFolder, const std::string& userDataFolder,
                       int width, int height)
{
    if (hWnd) { BringToFront(); return; }

    hInst = hInstance;
    UiFolderW = Utf8ToWide(uiFolder);
    UserDataFolderW = Utf8ToWide(userDataFolder);

    HRESULT coInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WND_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(15, 15, 18));
    RegisterClassW(&wc);  // ok if already registered

    // Center on screen
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - width) / 2;
    int y = (scrH - height) / 2;

    hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        WND_CLASS_NAME, title.c_str(),
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX,   // frameless but resizable
        x, y, width, height,
        NULL, NULL, hInstance, this);

    if (!hWnd) {
        LogMsg("WebViewHost: CreateWindow failed");
        if (SUCCEEDED(coInit)) CoUninitialize();
        return;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    CreateWebView();

    // Message loop until window destroyed
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Controller = nullptr;
    WebView = nullptr;
    hWnd = NULL;
    Ready = false;

    if (SUCCEEDED(coInit)) CoUninitialize();
}

void WebViewHost::CreateWebView()
{
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, UserDataFolderW.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    LogMsg("WebViewHost: environment creation failed hr=" + std::to_string(result));
                    return result;
                }
                env->CreateCoreWebView2Controller(hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) {
                                LogMsg("WebViewHost: controller creation failed hr=" + std::to_string(result));
                                return result;
                            }
                            Controller = controller;
                            Controller->get_CoreWebView2(&WebView);

                            // Dark background while loading
                            ComPtr<ICoreWebView2Controller2> ctrl2;
                            if (SUCCEEDED(Controller.As(&ctrl2))) {
                                COREWEBVIEW2_COLOR bg = { 255, 15, 15, 18 };
                                ctrl2->put_DefaultBackgroundColor(bg);
                            }

                            // Settings: disable devtools/context menu in release builds
                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(WebView->get_Settings(&settings))) {
                                settings->put_AreDefaultContextMenusEnabled(FALSE);
#ifdef _DEBUG
                                settings->put_AreDevToolsEnabled(TRUE);
#else
                                settings->put_AreDevToolsEnabled(FALSE);
#endif
                                settings->put_IsStatusBarEnabled(FALSE);
                                settings->put_IsZoomControlEnabled(FALSE);
                            }

                            // Map ui folder to virtual HTTPS host
                            ComPtr<ICoreWebView2_3> webView3;
                            if (SUCCEEDED(WebView.As(&webView3))) {
                                webView3->SetVirtualHostNameToFolderMapping(
                                    L"app.youtube-source", UiFolderW.c_str(),
                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                            }

                            // Receive messages from JS
                            WebView->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR raw = nullptr;
                                        if (SUCCEEDED(args->get_WebMessageAsJson(&raw)) && raw) {
                                            HandleWebMessage(raw);
                                            CoTaskMemFree(raw);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            ResizeWebView();
                            WebView->Navigate(L"https://app.youtube-source/index.html");
                            Ready = true;
                            LogMsg("WebViewHost: WebView2 initialized");
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        LogMsg("WebViewHost: CreateCoreWebView2EnvironmentWithOptions failed hr=" + std::to_string(hr));
        MessageBoxW(hWnd,
            L"Microsoft Edge WebView2 Runtime is required.\n\n"
            L"Please install it from:\nhttps://developer.microsoft.com/microsoft-edge/webview2/",
            L"YouTube Source", MB_OK | MB_ICONWARNING);
        DestroyWindow(hWnd);
    }
}

void WebViewHost::ResizeWebView()
{
    if (!Controller || !hWnd) return;
    RECT rc;
    GetClientRect(hWnd, &rc);
    Controller->put_Bounds(rc);
}

void WebViewHost::HandleWebMessage(const std::wstring& jsonW)
{
    std::string jsonStr = WideToUtf8(jsonW);
    try {
        json j = json::parse(jsonStr);
        std::string type = j.value("type", "");

        // Window chrome messages handled internally
        if (type == "dragWindow") {
            ReleaseCapture();
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return;
        }
        if (type == "closeWindow") {
            ShowWindow(hWnd, SW_HIDE);
            return;
        }
        if (type == "minimizeWindow") {
            ShowWindow(hWnd, SW_MINIMIZE);
            return;
        }
        if (type == "quitWindow") {
            DestroyWindow(hWnd);
            return;
        }

        std::string payload = j.contains("payload") ? j["payload"].dump() : "{}";
        if (OnMessage) OnMessage(type, payload);
    } catch (const std::exception& e) {
        LogMsg(std::string("WebViewHost: bad message: ") + e.what());
    }
}

LRESULT CALLBACK WebViewHost::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebViewHost* self = nullptr;
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        self = (WebViewHost*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (WebViewHost*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (!self) return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_SIZE:
        self->ResizeWebView();
        return 0;

    case WM_POST_JSON: {
        std::string* str = (std::string*)lParam;
        if (str) {
            if (self->WebView && self->Ready) {
                self->WebView->PostWebMessageAsJson(Utf8ToWide(*str).c_str());
            }
            delete str;
        }
        return 0;
    }

    case WM_CLOSE_REQUEST:
        DestroyWindow(hwnd);
        return 0;

    case WM_CLOSE:
        // Hide instead of destroy so the window can be re-shown quickly
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 720;
        mmi->ptMinTrackSize.y = 480;
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
