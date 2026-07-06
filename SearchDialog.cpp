#include "SearchDialog.h"
#include "PluginVersion.h"
#include <thread>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <chrono>
#include <iomanip>

const wchar_t* SearchDialog::WND_CLASS_NAME = L"YouTubeSearchDialog";

// Color palette - Modern dark theme with accent colors
static const COLORREF CLR_BG_DARK       = RGB(18, 18, 22);
static const COLORREF CLR_BG_MEDIUM     = RGB(25, 25, 30);
static const COLORREF CLR_BG_LIGHT      = RGB(35, 35, 42);
static const COLORREF CLR_BG_LIGHTER    = RGB(45, 45, 55);
static const COLORREF CLR_TITLEBAR      = RGB(15, 15, 18);
static const COLORREF CLR_SEARCH_BG     = RGB(30, 30, 38);
static const COLORREF CLR_ROW_EVEN      = RGB(22, 22, 28);
static const COLORREF CLR_ROW_ODD       = RGB(26, 26, 32);
static const COLORREF CLR_ROW_HOVER     = RGB(35, 35, 45);
static const COLORREF CLR_ROW_SELECT    = RGB(40, 50, 70);
static const COLORREF CLR_TEXT_PRIMARY   = RGB(230, 230, 235);
static const COLORREF CLR_TEXT_SECONDARY = RGB(140, 140, 155);
static const COLORREF CLR_TEXT_DIM       = RGB(90, 90, 105);
static const COLORREF CLR_ACCENT         = RGB(255, 0, 0);   // YouTube red
static const COLORREF CLR_ACCENT_HOVER   = RGB(255, 40, 80);
static const COLORREF CLR_DECK_A         = RGB(0, 150, 255);  // Blue for Deck A
static const COLORREF CLR_DECK_A_HOVER   = RGB(30, 170, 255);
static const COLORREF CLR_DECK_B         = RGB(255, 120, 0);  // Orange for Deck B
static const COLORREF CLR_DECK_B_HOVER   = RGB(255, 145, 30);
static const COLORREF CLR_PROGRESS_BG    = RGB(35, 35, 42);
static const COLORREF CLR_PROGRESS_FILL  = RGB(255, 0, 50);
static const COLORREF CLR_BORDER         = RGB(50, 50, 60);
static const COLORREF CLR_BORDER_FOCUS   = RGB(255, 0, 50);

SearchDialog::SearchDialog() {
    hBgBrush         = CreateSolidBrush(CLR_BG_DARK);
    hTopBarBrush     = CreateSolidBrush(CLR_TITLEBAR);
    hHeaderBrush     = CreateSolidBrush(CLR_BG_MEDIUM);
    hEditBgBrush     = CreateSolidBrush(CLR_SEARCH_BG);
    hRowEvenBrush    = CreateSolidBrush(CLR_ROW_EVEN);
    hRowOddBrush     = CreateSolidBrush(CLR_ROW_ODD);
    hHighlightBrush  = CreateSolidBrush(CLR_ROW_SELECT);
    hBtnBrush        = CreateSolidBrush(CLR_BG_LIGHTER);
    hBtnHoverBrush   = CreateSolidBrush(RGB(55, 55, 68));
    hAccentBrush     = CreateSolidBrush(CLR_ACCENT);
    hProgressBgBrush = CreateSolidBrush(CLR_PROGRESS_BG);
    hProgressFillBrush = CreateSolidBrush(CLR_PROGRESS_FILL);
    hDeckABrush      = CreateSolidBrush(CLR_DECK_A);
    hDeckBBrush      = CreateSolidBrush(CLR_DECK_B);
    hAutomixBrush    = CreateSolidBrush(RGB(140, 80, 200));
    hSidelistBrush   = CreateSolidBrush(RGB(60, 140, 180));
    hFormatMp3Brush  = CreateSolidBrush(RGB(40, 160, 80));
    hFormatMp4Brush  = CreateSolidBrush(RGB(180, 60, 60));
    hBtnBorderPen    = CreatePen(PS_SOLID, 1, CLR_BORDER);
    hSeparatorPen    = CreatePen(PS_SOLID, 1, RGB(40, 40, 50));
    hEditBorderPen   = CreatePen(PS_SOLID, 1, CLR_BORDER);
    hEditBorderFocusPen = CreatePen(PS_SOLID, 2, CLR_BORDER_FOCUS);
    hAccentPen       = CreatePen(PS_SOLID, 2, CLR_ACCENT);
    UpdateScaling();
    
    // Load logo from DLL directory (will be set later via SetToolsDirectory or similar)
    // Logo will be loaded when path is available
    hLogoBitmap = NULL;
    
    // Initialize logging
    InitializeLogging();
}

int SearchDialog::Scale(int value) const {
    return MulDiv(value, ScreenDPI, 96);
}

void SearchDialog::UpdateScaling() {
    HDC hdc = GetDC(NULL);
    if (hdc) {
        ScreenDPI = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }
}

void SearchDialog::UpdateFonts() {
    if (hFont) DeleteObject(hFont);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontSmall) DeleteObject(hFontSmall);
    if (hFontHeader) DeleteObject(hFontHeader);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontIcon) DeleteObject(hFontIcon);

    hFont = CreateFontA(-Scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    hFontBold = CreateFontA(-Scale(14), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    hFontSmall = CreateFontA(-Scale(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    hFontHeader = CreateFontA(-Scale(12), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    hFontTitle = CreateFontA(-Scale(13), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    hFontIcon = CreateFontW(-Scale(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

    if (hSearchBox) SendMessage(hSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (hSearchBtn) SendMessage(hSearchBtn, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    if (hResultsList) SendMessage(hResultsList, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (hLoadDeckA) SendMessage(hLoadDeckA, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    if (hLoadDeckB) SendMessage(hLoadDeckB, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    if (hStatusLabel) SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
    if (hProgressLabel) SendMessage(hProgressLabel, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
    if (hTitleLabel) SendMessage(hTitleLabel, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
}

SearchDialog::~SearchDialog() {
    // Uninitialize COM
    CoUninitialize();
    
    if (hFont) DeleteObject(hFont);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontSmall) DeleteObject(hFontSmall);
    if (hFontHeader) DeleteObject(hFontHeader);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontIcon) DeleteObject(hFontIcon);
    if (hLogoBitmap) DeleteObject(hLogoBitmap);
    if (hBgBrush) DeleteObject(hBgBrush);
    if (hTopBarBrush) DeleteObject(hTopBarBrush);
    if (hHeaderBrush) DeleteObject(hHeaderBrush);
    if (hEditBgBrush) DeleteObject(hEditBgBrush);
    if (hRowEvenBrush) DeleteObject(hRowEvenBrush);
    if (hRowOddBrush) DeleteObject(hRowOddBrush);
    if (hHighlightBrush) DeleteObject(hHighlightBrush);
    if (hBtnBrush) DeleteObject(hBtnBrush);
    if (hBtnHoverBrush) DeleteObject(hBtnHoverBrush);
    if (hAccentBrush) DeleteObject(hAccentBrush);
    if (hProgressBgBrush) DeleteObject(hProgressBgBrush);
    if (hProgressFillBrush) DeleteObject(hProgressFillBrush);
    if (hDeckABrush) DeleteObject(hDeckABrush);
    if (hDeckBBrush) DeleteObject(hDeckBBrush);
    if (hAutomixBrush) DeleteObject(hAutomixBrush);
    if (hSidelistBrush) DeleteObject(hSidelistBrush);
    if (hFormatMp3Brush) DeleteObject(hFormatMp3Brush);
    if (hFormatMp4Brush) DeleteObject(hFormatMp4Brush);
    if (hBtnBorderPen) DeleteObject(hBtnBorderPen);
    if (hSeparatorPen) DeleteObject(hSeparatorPen);
    if (hEditBorderPen) DeleteObject(hEditBorderPen);
    if (hEditBorderFocusPen) DeleteObject(hEditBorderFocusPen);
    if (hAccentPen) DeleteObject(hAccentPen);
    if (pLogo) {
        delete pLogo;
        pLogo = nullptr;
    }
    if (hWnd) {
        DestroyWindow(hWnd);
    }
}

void SearchDialog::LoadLogo() {
    if (pLogo) return; // Already loaded
    
    // Get the DLL directory to find the PNG next to it
    wchar_t dllPath[MAX_PATH] = {};
    GetModuleFileNameW(hInst, dllPath, MAX_PATH);
    std::wstring dir(dllPath);
    size_t pos = dir.find_last_of(L"\\//");
    if (pos != std::wstring::npos) dir = dir.substr(0, pos);
    
    std::wstring logoPath = dir + L"\\YouTubeSource.png";
    pLogo = Gdiplus::Image::FromFile(logoPath.c_str());
    if (pLogo && pLogo->GetLastStatus() != Gdiplus::Ok) {
        delete pLogo;
        pLogo = nullptr;
    }
}

void SearchDialog::SetToolsDirectory(const std::string& dir) {
    ToolsDir = dir;
    OutputDebugStringA(("SetToolsDirectory called with: " + dir + "\n").c_str());
    
    // Load logo from same directory as DLL (same as ToolsDir)
    if (!hLogoBitmap && !dir.empty()) {
        LogoPath = dir + "\\YouTubeSource.png";
        OutputDebugStringA(("Attempting to load logo from: " + LogoPath + "\n").c_str());
        
        // Check if file exists
        std::ifstream file(LogoPath);
        bool fileExists = file.good();
        file.close();
        OutputDebugStringA(("Logo file exists: " + std::string(fileExists ? "YES" : "NO") + "\n").c_str());
        
        if (fileExists) {
            std::wstring wLogoPath(LogoPath.begin(), LogoPath.end());

            // Load original image for custom title bar painting if not already loaded
            if (!pLogo) {
                pLogo = Gdiplus::Image::FromFile(wLogoPath.c_str());
                if (!pLogo || pLogo->GetLastStatus() != Gdiplus::Ok) {
                    OutputDebugStringA("Failed to load pLogo image for title bar\n");
                    if (pLogo) {
                        delete pLogo;
                        pLogo = nullptr;
                    }
                } else {
                    OutputDebugStringA("Loaded pLogo image for title bar\n");
                }
            }

            // Use GDI+ to load PNG file and resize to 40x40 for bitmap controls
            Gdiplus::Bitmap* pOriginal = Gdiplus::Bitmap::FromFile(wLogoPath.c_str());
            if (pOriginal && pOriginal->GetLastStatus() == Gdiplus::Ok) {
                // Create a 40x40 resized version
                Gdiplus::Bitmap* pResized = new Gdiplus::Bitmap(40, 40, PixelFormat32bppARGB);
                Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromImage(pResized);
                if (pGraphics) {
                    pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                    pGraphics->DrawImage(pOriginal, 0, 0, 40, 40);
                    delete pGraphics;
                }
                pResized->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hLogoBitmap);
                delete pResized;
                delete pOriginal;
                OutputDebugStringA(("Logo loaded and resized to 40x40 from: " + LogoPath + "\n").c_str());
            } else {
                OutputDebugStringA("Failed to load PNG logo with GDI+\n");
                if (pOriginal) delete pOriginal;
            }
        }
    }
}

void SearchDialog::SetCacheDirectory(const std::string& dir) {
    CacheDir = dir;
}

void SearchDialog::SetLoadCallback(LoadCallback callback) {
    OnLoadTrack = callback;
}

void SearchDialog::SetStreamCallback(StreamCallback callback) {
    OnStreamTrack = callback;
}

void SearchDialog::SetSearchCountCallback(SearchCountCallback callback) {
    OnSearchCount = callback;
}

void SearchDialog::SetFormatChangeCallback(FormatChangeCallback callback) {
    OnFormatChange = callback;
}

void SearchDialog::SetDownloadVideoCallback(DownloadVideoCallback callback) {
    OnDownloadVideo = callback;
}

void SearchDialog::SetLicenseRequiredCallback(LicenseRequiredCallback callback) {
    OnLicenseRequired = callback;
}

void SearchDialog::SetFormat(const std::string& format) {
    CurrentFormat = format;
    if (hFormatBtn) {
        std::string label = (CurrentFormat == "mp4") ? "MP4" : "MP3";
        SetWindowTextA(hFormatBtn, label.c_str());
        InvalidateRect(hFormatBtn, NULL, TRUE);
    }
}

std::string SearchDialog::GetFormat() const {
    return CurrentFormat;
}

void SearchDialog::SetStatusText(const std::string& text) {
    if (hStatusLabel) {
        SetWindowTextA(hStatusLabel, text.c_str());
    }
}

void SearchDialog::SetLicenseStatus(bool licensed, int searchesUsed, int searchesMax, const std::string& expiry, const std::string& machineID) {
    LogInfo("SetLicenseStatus called: licensed=" + std::string(licensed ? "true" : "false") + 
            ", expiry=" + expiry + ", machineID=" + machineID);
    
    LicenseValid   = licensed;
    SearchesUsed   = searchesUsed;
    SearchesMax    = searchesMax;
    LicenseExpiry  = expiry;
    MachineID      = machineID;

    // If window exists, use PostMessage for thread-safe update
    if (hWnd && IsWindow(hWnd)) {
        LogInfo("Posting WM_LICENSE_STATUS_CHANGED message to search dialog window");
        PostMessage(hWnd, WM_LICENSE_STATUS_CHANGED, 0, 0);
    } else {
        LogWarning("Search dialog window not created yet - license status will be applied when window is shown");
    }
}

void SearchDialog::RefreshLicenseStatus() {
    LogInfo("RefreshLicenseStatus called: LicenseValid=" + std::string(LicenseValid ? "true" : "false") + 
            ", hWnd=" + (hWnd ? "valid" : "NULL"));
    
    if (!hWnd) {
        LogWarning("Cannot refresh license status - hWnd is NULL");
        return;
    }

    // Update title label (include version) - two states only
    std::string title;
    if (LicenseValid) {
        title = std::string("  YouTube Search v") + PLUGIN_VERSION + " - Pro";
    } else {
        title = std::string("  YouTube Search v") + PLUGIN_VERSION + " - Unlicensed";
    }
    
    if (hTitleLabel) {
        SetWindowTextA(hTitleLabel, title.c_str());
    }

    // Repaint title bar + bottom panel so badge and counter update
    RECT rc;
    GetClientRect(hWnd, &rc);
    RECT titleRc = { 0, 0, rc.right, Scale(38) };
    RECT bottomRc = { 0, rc.bottom - Scale(70), rc.right, rc.bottom };
    InvalidateRect(hWnd, &titleRc, FALSE);
    InvalidateRect(hWnd, &bottomRc, FALSE);
}

void SearchDialog::SetDownloadProgress(const DownloadProgress& progress) {
    CurrentProgress = progress;
    if (hWnd) {
        // Trigger repaint of progress area
        RECT rc;
        GetClientRect(hWnd, &rc);
        rc.top = rc.bottom - Scale(70);
        InvalidateRect(hWnd, &rc, FALSE);
        // Update progress label text
        if (hProgressLabel && progress.active) {
            std::string info = progress.phase;
            if (!progress.speed.empty()) info += "  |  " + progress.speed;
            if (!progress.eta.empty()) info += "  |  ETA: " + progress.eta;
            if (progress.percent > 0) {
                char pct[16];
                snprintf(pct, sizeof(pct), "  |  %.0f%%", progress.percent);
                info += pct;
            }
            SetWindowTextA(hProgressLabel, info.c_str());
        } else if (hProgressLabel) {
            SetWindowTextA(hProgressLabel, "");
        }
    }
}

bool SearchDialog::IsVisible() const {
    return hWnd && IsWindowVisible(hWnd);
}

void SearchDialog::Show(HINSTANCE hInstance) {
    LogInfo("SearchDialog::Show called");
    
    if (hWnd) {
        LogInfo("SearchDialog window already exists, bringing to front");
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        return;
    }

    // Initialize COM for this thread (required for some Windows operations)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        LogWarning("COM initialization failed with HRESULT: " + std::to_string(hr));
    } else {
        LogInfo("COM initialized successfully in SearchDialog");
    }

    hInst = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = hBgBrush;
    wc.lpszClassName = WND_CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    UpdateScaling();
    UpdateFonts();

    int width = Scale(960);
    int height = Scale(680);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    // Borderless popup - we draw our own title bar
    hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        WND_CLASS_NAME,
        L"",
        WS_POPUP | WS_SIZEBOX | WS_CLIPCHILDREN,
        x, y, width, height,
        NULL, NULL, hInstance, this
    );

    if (!hWnd) {
        DWORD error = GetLastError();
        LogError("Failed to create search dialog window. Error code: " + std::to_string(error));
        return;
    }

    LogInfo("Search dialog window created successfully at: " + std::to_string(x) + ", " + std::to_string(y));

    CreateControls();
    LoadLogo();
    
    // Force window visibility
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    BringWindowToTop(hWnd);
    SetActiveWindow(hWnd);
    SetFocus(hSearchBox);
    
    LogInfo("Search dialog shown and focused");
    
    // Trigger license status refresh after window is fully created
    PostMessage(hWnd, WM_USER + 100, 0, 0);
    LogInfo("Search dialog window created and shown");
    
    // Message loop - keeps window alive until closed
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsWindow(hWnd)) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    LogInfo("Search dialog message loop ended");
    CoUninitialize();
}

void SearchDialog::Hide() {
    if (hWnd) ShowWindow(hWnd, SW_HIDE);
}

// Subclass for search box to intercept Enter key
LRESULT CALLBACK SearchDialog::SearchBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SearchDialog* dlg = (SearchDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        if (dlg) dlg->OnSearch();
        return 0;
    }
    if (dlg && dlg->OldSearchBoxProc)
        return CallWindowProc(dlg->OldSearchBoxProc, hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SearchDialog::CreateControls() {
    int m = Scale(12);           // margin
    int titleH = Scale(38);     // custom title bar height
    int searchH = Scale(36);    // search bar height
    int bottomH = Scale(64);    // bottom panel height
    int btnH = Scale(30);

    // === Custom Title Bar ===
    int logoSize = Scale(28);
    int logoX = Scale(8);
    int logoY = (titleH - logoSize) / 2;  // vertically centered in title bar

    // Logo at top-left like YouTube dialog
    if (hLogoBitmap) {
        OutputDebugStringA("Creating logo control in title bar\n");
        HWND hLogo = CreateWindowExA(
            0, "STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
            logoX, logoY, logoSize, logoSize,
            hWnd, NULL, hInst, NULL
        );
        SendMessageA(hLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hLogoBitmap);
    } else {
        OutputDebugStringA("No logo bitmap available - logo not displayed\n");
    }

    // Title text to the right of the logo
    std::string title = "  YouTube Search v" PLUGIN_VERSION;
    hTitleLabel = CreateWindowExA(
        0, "STATIC", title.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        logoX + logoSize + Scale(8), 0, Scale(320), titleH,
        hWnd, (HMENU)(INT_PTR)ID_TITLE_LABEL, hInst, NULL
    );

    hCloseBtn = CreateWindowExA(
        0, "BUTTON", "X",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        Scale(900), 0, Scale(46), titleH,
        hWnd, (HMENU)(INT_PTR)ID_CLOSE_BTN, hInst, NULL
    );

    // === Search Bar Area ===
    int searchY = titleH + m;

    hSearchBox = CreateWindowExA(
        0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        m, searchY + Scale(4), Scale(600), searchH - Scale(8),
        hWnd, (HMENU)(INT_PTR)ID_SEARCH_BOX, hInst, NULL
    );
    SendMessageW(hSearchBox, EM_SETCUEBANNER, FALSE, (LPARAM)L"Search YouTube...");

    // Subclass search box for Enter key
    SetWindowLongPtr(hSearchBox, GWLP_USERDATA, (LONG_PTR)this);
    OldSearchBoxProc = (WNDPROC)SetWindowLongPtr(hSearchBox, GWLP_WNDPROC, (LONG_PTR)SearchBoxProc);

    hSearchBtn = CreateWindowExA(
        0, "BUTTON", "SEARCH",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        Scale(670), searchY, Scale(100), searchH,
        hWnd, (HMENU)(INT_PTR)ID_SEARCH_BTN, hInst, NULL
    );

    hStatusLabel = CreateWindowExA(
        0, "STATIC", "Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(780), searchY + Scale(8), Scale(160), Scale(20),
        hWnd, (HMENU)(INT_PTR)ID_STATUS_LABEL, hInst, NULL
    );

    // === Results ListView ===
    int listY = searchY + searchH + m;

    hResultsList = CreateWindowExW(
        0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
        0, listY, Scale(960), Scale(400),
        hWnd, (HMENU)(INT_PTR)ID_RESULTS_LIST, hInst, NULL
    );

    ListView_SetExtendedListViewStyle(hResultsList,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    ListView_SetBkColor(hResultsList, CLR_ROW_EVEN);
    ListView_SetTextBkColor(hResultsList, CLR_ROW_EVEN);
    ListView_SetTextColor(hResultsList, CLR_TEXT_PRIMARY);

    // Columns: #, Title, Channel, Duration
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.cx = Scale(36);
    col.pszText = (LPWSTR)L"#";
    col.iSubItem = 0;
    ListView_InsertColumn(hResultsList, 0, &col);

    col.cx = Scale(480);
    col.pszText = (LPWSTR)L"TITLE";
    col.iSubItem = 1;
    ListView_InsertColumn(hResultsList, 1, &col);

    col.cx = Scale(260);
    col.pszText = (LPWSTR)L"CHANNEL";
    col.iSubItem = 2;
    ListView_InsertColumn(hResultsList, 2, &col);

    col.cx = Scale(80);
    col.pszText = (LPWSTR)L"DURATION";
    col.iSubItem = 3;
    ListView_InsertColumn(hResultsList, 3, &col);

    // === Bottom Panel: Progress + Deck Buttons ===
    int bottomY = Scale(580);

    hProgressLabel = CreateWindowExA(
        0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        m, bottomY + Scale(4), Scale(600), Scale(16),
        hWnd, (HMENU)(INT_PTR)ID_PROGRESS_LABEL, hInst, NULL
    );

    int deckBtnW = Scale(110);
    int deckBtnSpacing = Scale(10);
    hLoadDeckA = CreateWindowExA(
        0, "BUTTON", "DECK A",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        m, bottomY + Scale(22), deckBtnW, btnH,
        hWnd, (HMENU)(INT_PTR)ID_LOAD_DECK_A, hInst, NULL
    );

    hLoadDeckB = CreateWindowExA(
        0, "BUTTON", "DECK B",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        m + deckBtnW + deckBtnSpacing, bottomY + Scale(22), deckBtnW, btnH,
        hWnd, (HMENU)(INT_PTR)ID_LOAD_DECK_B, hInst, NULL
    );

    hAddAutomix = CreateWindowExA(
        0, "BUTTON", "AUTOMIX",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        m + (deckBtnW + deckBtnSpacing) * 2, bottomY + Scale(22), deckBtnW, btnH,
        hWnd, (HMENU)(INT_PTR)ID_ADD_AUTOMIX, hInst, NULL
    );

    hAddSidelist = CreateWindowExA(
        0, "BUTTON", "SIDELIST",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        m + (deckBtnW + deckBtnSpacing) * 3, bottomY + Scale(22), deckBtnW, btnH,
        hWnd, (HMENU)(INT_PTR)ID_ADD_SIDELIST, hInst, NULL
    );

    std::string fmtLabel = (CurrentFormat == "mp4") ? "MP4" : "MP3";
    int fmtBtnW = Scale(60);
    hFormatBtn = CreateWindowExA(
        0, "BUTTON", fmtLabel.c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        m + (deckBtnW + deckBtnSpacing) * 4, bottomY + Scale(22), fmtBtnW, btnH,
        hWnd, (HMENU)(INT_PTR)ID_FORMAT_BTN, hInst, NULL
    );

    // Subclass header for custom drawing
    hHeader = ListView_GetHeader(hResultsList);
    if (hHeader) {
        SetWindowLongPtr(hHeader, GWLP_USERDATA, (LONG_PTR)this);
        OldHeaderWndProc = (WNDPROC)SetWindowLongPtr(hHeader, GWLP_WNDPROC, (LONG_PTR)HeaderWndProc);
    }

    UpdateFonts();

    RECT rc;
    GetClientRect(hWnd, &rc);
    ResizeControls(rc.right, rc.bottom);
}


void SearchDialog::ResizeControls(int w, int h) {
    int m = Scale(12);
    int titleH = Scale(38);
    int searchH = Scale(36);
    int btnH = Scale(30);
    int bottomH = Scale(64);

    // Title bar
    if (hTitleLabel) MoveWindow(hTitleLabel, 0, 0, w - Scale(46), titleH, TRUE);
    if (hCloseBtn) MoveWindow(hCloseBtn, w - Scale(46), 0, Scale(46), titleH, TRUE);

    // Search area
    int searchY = titleH + m;
    int searchBtnW = Scale(100);
    int statusW = Scale(160);
    int searchBoxW = w - m - Scale(40) - m - searchBtnW - m - statusW - m;
    if (searchBoxW < Scale(200)) searchBoxW = Scale(200);

    if (hSearchBox) MoveWindow(hSearchBox, m + Scale(40), searchY + Scale(4), searchBoxW, searchH - Scale(8), TRUE);
    if (hSearchBtn) MoveWindow(hSearchBtn, m + Scale(40) + searchBoxW + m, searchY, searchBtnW, searchH, TRUE);
    if (hStatusLabel) MoveWindow(hStatusLabel, w - statusW - m, searchY + Scale(8), statusW, Scale(20), TRUE);

    // ListView fills middle area
    int listY = searchY + searchH + m;
    int listH = h - listY - bottomH;
    if (listH < Scale(100)) listH = Scale(100);
    if (hResultsList) MoveWindow(hResultsList, 0, listY, w, listH, TRUE);

    // Bottom panel
    int bottomY = h - bottomH;
    int deckBtnW = Scale(110);
    int deckBtnSpacing = Scale(10);
    if (hProgressLabel) MoveWindow(hProgressLabel, m, bottomY + Scale(4), w - m * 2, Scale(16), TRUE);
    if (hLoadDeckA) MoveWindow(hLoadDeckA, m, bottomY + Scale(22), deckBtnW, btnH, TRUE);
    if (hLoadDeckB) MoveWindow(hLoadDeckB, m + deckBtnW + deckBtnSpacing, bottomY + Scale(22), deckBtnW, btnH, TRUE);
    if (hAddAutomix) MoveWindow(hAddAutomix, m + (deckBtnW + deckBtnSpacing) * 2, bottomY + Scale(22), deckBtnW, btnH, TRUE);
    if (hAddSidelist) MoveWindow(hAddSidelist, m + (deckBtnW + deckBtnSpacing) * 3, bottomY + Scale(22), deckBtnW, btnH, TRUE);
    int fmtBtnW = Scale(60);
    if (hFormatBtn) MoveWindow(hFormatBtn, m + (deckBtnW + deckBtnSpacing) * 4, bottomY + Scale(22), fmtBtnW, btnH, TRUE);

    // Auto-size columns
    if (hResultsList) {
        int col0 = Scale(36);
        int col3 = Scale(80);
        int col2 = (w * 25) / 100;
        int col1 = w - col0 - col2 - col3 - GetSystemMetrics(SM_CXVSCROLL) - 4;
        if (col1 < Scale(150)) col1 = Scale(150);
        ListView_SetColumnWidth(hResultsList, 0, col0);
        ListView_SetColumnWidth(hResultsList, 1, col1);
        ListView_SetColumnWidth(hResultsList, 2, col2);
        ListView_SetColumnWidth(hResultsList, 3, col3);
    }
}

void SearchDialog::DrawProgressBar(HDC hdc, RECT rc) {
    if (!CurrentProgress.active) return;

    // Background
    HBRUSH bgBr = CreateSolidBrush(CLR_PROGRESS_BG);
    FillRect(hdc, &rc, bgBr);
    DeleteObject(bgBr);

    // Fill
    if (CurrentProgress.percent > 0) {
        RECT fillRc = rc;
        fillRc.right = fillRc.left + (int)((float)(rc.right - rc.left) * CurrentProgress.percent / 100.0f);
        HBRUSH fillBr = CreateSolidBrush(CLR_PROGRESS_FILL);
        FillRect(hdc, &fillRc, fillBr);
        DeleteObject(fillBr);
    }

    // Border
    HPEN pen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 4, 4);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

LRESULT CALLBACK SearchDialog::HeaderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SearchDialog* dlg = (SearchDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!dlg) return DefWindowProc(hwnd, msg, wParam, lParam);

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        FillRect(hdc, &clientRect, dlg->hHeaderBrush);

        int count = Header_GetItemCount(hwnd);
        SelectObject(hdc, dlg->hFontHeader);
        SetTextColor(hdc, CLR_TEXT_DIM);
        SetBkMode(hdc, TRANSPARENT);

        for (int i = 0; i < count; i++) {
            RECT itemRect;
            Header_GetItemRect(hwnd, i, &itemRect);

            wchar_t text[128];
            HDITEMW hdi = {};
            hdi.mask = HDI_TEXT;
            hdi.pszText = text;
            hdi.cchTextMax = 128;
            SendMessageW(hwnd, HDM_GETITEMW, (WPARAM)i, (LPARAM)&hdi);

            RECT textRect = itemRect;
            textRect.left += dlg->Scale(8);
            DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        // Bottom accent line
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(40, 40, 50));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 0, clientRect.bottom - 1, NULL);
        LineTo(hdc, clientRect.right, clientRect.bottom - 1);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        EndPaint(hwnd, &ps);
        return 0;
    }

    return CallWindowProc(dlg->OldHeaderWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SearchDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SearchDialog* dlg = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        dlg = (SearchDialog*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)dlg);
    } else {
        dlg = (SearchDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (!dlg) return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
    // Custom title bar hit testing - allow dragging from title area
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            int titleH = dlg->Scale(38);
            if (pt.y < titleH && pt.x < (int)(GetSystemMetrics(SM_CXSCREEN)) - dlg->Scale(46)) {
                return HTCAPTION;  // Allow dragging from title bar
            }
        }
        return hit;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        int titleH = dlg->Scale(38);
        int m = dlg->Scale(12);
        int searchY = titleH + m;
        int searchH = dlg->Scale(36);

        // === Title bar ===
        RECT titleRc = { 0, 0, rc.right, titleH };
        FillRect(hdc, &titleRc, dlg->hTopBarBrush);

        // === YouTube logo in title bar ===
        int logoOffset = 0;
        if (dlg->pLogo) {
            int logoH = titleH - dlg->Scale(8); // Padding
            int logoW = (int)((float)dlg->pLogo->GetWidth() / dlg->pLogo->GetHeight() * logoH);
            int logoX = dlg->Scale(8);
            int logoY = (titleH - logoH) / 2;
            
            Gdiplus::Graphics gfx(hdc);
            gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            gfx.DrawImage(dlg->pLogo, logoX, logoY, logoW, logoH);
            logoOffset = logoX + logoW + dlg->Scale(4);
        }

        // Reposition title label after logo
        if (dlg->hTitleLabel && logoOffset > 0) {
            RECT tlRc;
            GetWindowRect(dlg->hTitleLabel, &tlRc);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&tlRc, 2);
            SetWindowPos(dlg->hTitleLabel, NULL, logoOffset, tlRc.top, tlRc.right - logoOffset, tlRc.bottom - tlRc.top, SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Red accent line under title
        HPEN accentPen = CreatePen(PS_SOLID, 2, CLR_ACCENT);
        HPEN oldPen = (HPEN)SelectObject(hdc, accentPen);
        MoveToEx(hdc, 0, titleH - 1, NULL);
        LineTo(hdc, rc.right, titleH - 1);
        SelectObject(hdc, oldPen);
        DeleteObject(accentPen);

        // === License badge in title bar (right side, left of close button) ===
        {
            const bool lic = dlg->LicenseValid;
            const wchar_t* badgeText = lic ? L"  PRO  " : L"  UNLICENSED  ";
            COLORREF badgeColor = lic ? RGB(30, 180, 80) : RGB(220, 140, 0);
            int closeBtnW = dlg->Scale(46);
            int badgePadY = dlg->Scale(7);

            // Measure badge text width
            HFONT oldFont = (HFONT)SelectObject(hdc, dlg->hFontSmall);
            SIZE sz = {};
            GetTextExtentPoint32W(hdc, badgeText, (int)wcslen(badgeText), &sz);

            int badgeX = rc.right - closeBtnW - sz.cx - dlg->Scale(10);
            int badgeY = badgePadY;
            int badgeW = sz.cx;
            int badgeH = titleH - badgePadY * 2;

            // Filled pill background
            HBRUSH badgeBr = CreateSolidBrush(badgeColor);
            HPEN badgePen  = CreatePen(PS_SOLID, 1, badgeColor);
            HPEN oBadgePen = (HPEN)SelectObject(hdc, badgePen);
            HBRUSH oBadgeBr = (HBRUSH)SelectObject(hdc, badgeBr);
            RoundRect(hdc, badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, badgeH, badgeH);
            SelectObject(hdc, oBadgePen);
            SelectObject(hdc, oBadgeBr);
            DeleteObject(badgePen);
            DeleteObject(badgeBr);

            // Badge text
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT badgeRc = { badgeX, badgeY, badgeX + badgeW, badgeY + badgeH };
            DrawTextW(hdc, badgeText, -1, &badgeRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, oldFont);
        }

        // === Search area background ===
        RECT searchAreaRc = { 0, titleH, rc.right, searchY + searchH + m };
        FillRect(hdc, &searchAreaRc, dlg->hBgBrush);

        // Search icon (magnifying glass drawn as text)
        SelectObject(hdc, dlg->hFontIcon);
        SetTextColor(hdc, CLR_TEXT_SECONDARY);
        SetBkMode(hdc, TRANSPARENT);
        RECT iconRc = { m, searchY + dlg->Scale(2), m + dlg->Scale(36), searchY + searchH };
        DrawTextW(hdc, L"\xD83D\xDD0D", -1, &iconRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Search box border with rounded feel
        if (dlg->hSearchBox) {
            RECT sRc;
            GetWindowRect(dlg->hSearchBox, &sRc);
            ScreenToClient(hwnd, (LPPOINT)&sRc.left);
            ScreenToClient(hwnd, (LPPOINT)&sRc.right);
            InflateRect(&sRc, 2, 2);

            HPEN borderPen = CreatePen(PS_SOLID, dlg->EditFocused ? 2 : 1,
                dlg->EditFocused ? CLR_ACCENT : CLR_BORDER);
            HPEN oPen = (HPEN)SelectObject(hdc, borderPen);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, sRc.left, sRc.top, sRc.right, sRc.bottom, 6, 6);
            SelectObject(hdc, oPen);
            DeleteObject(borderPen);
        }

        // === Bottom panel background ===
        int bottomH = dlg->Scale(70);
        RECT bottomRc = { 0, rc.bottom - bottomH, rc.right, rc.bottom };
        FillRect(hdc, &bottomRc, dlg->hBgBrush);

        // Separator above bottom
        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(40, 40, 50));
        oldPen = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, 0, rc.bottom - bottomH, NULL);
        LineTo(hdc, rc.right, rc.bottom - bottomH);
        SelectObject(hdc, oldPen);
        DeleteObject(sepPen);

        // Progress bar in bottom panel
        if (dlg->CurrentProgress.active) {
            int deckBtnW = dlg->Scale(140);
            RECT progRc = {
                m * 3 + deckBtnW * 2,
                rc.bottom - bottomH + dlg->Scale(30),
                rc.right - m,
                rc.bottom - m
            };
            dlg->DrawProgressBar(hdc, progRc);
        }

        // === Searches remaining counter (bottom right, unlicensed only) ===
        if (!dlg->LicenseValid) {
            int left = dlg->SearchesMax - dlg->SearchesUsed;
            if (left < 0) left = 0;

            // Searches remaining (top line)
            wchar_t searchBuf[64];
            swprintf(searchBuf, 64, L"%d / %d free searches remaining", left, dlg->SearchesMax);

            COLORREF counterColor = left <= 1 ? RGB(220, 60, 60)
                                  : left <= 2 ? RGB(220, 140, 0)
                                  : RGB(160, 160, 180);

            HFONT oldF = (HFONT)SelectObject(hdc, dlg->hFontSmall);
            SIZE sz2 = {};
            GetTextExtentPoint32W(hdc, searchBuf, (int)wcslen(searchBuf), &sz2);

            int cx = rc.right - m - sz2.cx;
            int cy = rc.bottom - m - sz2.cy - dlg->Scale(16); // Move up to make room for Machine ID
            SetTextColor(hdc, counterColor);
            SetBkMode(hdc, TRANSPARENT);
            TextOutW(hdc, cx, cy, searchBuf, (int)wcslen(searchBuf));

            // Machine ID (bottom line, always shown for unlicensed users)
            if (!dlg->MachineID.empty()) {
                wchar_t machineBuf[64];
                std::wstring machineIDStr(dlg->MachineID.begin(), dlg->MachineID.end());
                swprintf(machineBuf, 64, L"Machine ID: %s", machineIDStr.c_str());

                COLORREF machineColor = RGB(100, 100, 120);
                SetTextColor(hdc, machineColor);
                
                cy += dlg->Scale(16); // Position below searches counter
                TextOutW(hdc, cx, cy, machineBuf, (int)wcslen(machineBuf));
            }
            
            SelectObject(hdc, oldF);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        if (lpdis->CtlType == ODT_BUTTON) {
            int ctlId = (int)lpdis->CtlID;
            bool isPressed = (lpdis->itemState & ODS_SELECTED) != 0;
            bool isDisabled = (lpdis->itemState & ODS_DISABLED) != 0;

            // Check hover
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(lpdis->hwndItem, &pt);
            RECT crc;
            GetClientRect(lpdis->hwndItem, &crc);
            bool isHover = PtInRect(&crc, pt) != 0;

            COLORREF bgColor, textColor;
            int cornerRadius = 6;

            if (ctlId == ID_SEARCH_BTN) {
                bgColor = isPressed ? RGB(200, 0, 40) : (isHover ? CLR_ACCENT_HOVER : CLR_ACCENT);
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_LOAD_DECK_A) {
                bgColor = isPressed ? RGB(0, 120, 220) : (isHover ? CLR_DECK_A_HOVER : CLR_DECK_A);
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_LOAD_DECK_B) {
                bgColor = isPressed ? RGB(220, 100, 0) : (isHover ? CLR_DECK_B_HOVER : CLR_DECK_B);
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_ADD_AUTOMIX) {
                bgColor = isPressed ? RGB(100, 50, 160) : (isHover ? RGB(160, 100, 220) : RGB(140, 80, 200));
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_ADD_SIDELIST) {
                bgColor = isPressed ? RGB(40, 110, 150) : (isHover ? RGB(80, 160, 200) : RGB(60, 140, 180));
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_FORMAT_BTN) {
                if (dlg->CurrentFormat == "mp4") {
                    bgColor = isPressed ? RGB(140, 40, 40) : (isHover ? RGB(200, 80, 80) : RGB(180, 60, 60));
                } else {
                    bgColor = isPressed ? RGB(30, 120, 60) : (isHover ? RGB(60, 180, 100) : RGB(40, 160, 80));
                }
                textColor = RGB(255, 255, 255);
            } else if (ctlId == ID_CLOSE_BTN) {
                bgColor = isHover ? RGB(200, 30, 30) : CLR_TITLEBAR;
                textColor = CLR_TEXT_PRIMARY;
                cornerRadius = 0;
            } else {
                bgColor = isHover ? RGB(55, 55, 68) : CLR_BG_LIGHTER;
                textColor = CLR_TEXT_PRIMARY;
            }

            if (isDisabled) {
                bgColor = CLR_BG_LIGHT;
                textColor = CLR_TEXT_DIM;
            }

            // Fill with rounded corners
            HBRUSH br = CreateSolidBrush(bgColor);
            HPEN pen = CreatePen(PS_SOLID, 1, bgColor);
            HPEN oPen = (HPEN)SelectObject(lpdis->hDC, pen);
            HBRUSH oBr = (HBRUSH)SelectObject(lpdis->hDC, br);
            RoundRect(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
                       lpdis->rcItem.right, lpdis->rcItem.bottom, cornerRadius, cornerRadius);
            SelectObject(lpdis->hDC, oPen);
            SelectObject(lpdis->hDC, oBr);
            DeleteObject(br);
            DeleteObject(pen);

            // Text
            char text[128];
            GetWindowTextA(lpdis->hwndItem, text, sizeof(text));
            SetTextColor(lpdis->hDC, textColor);
            SetBkMode(lpdis->hDC, TRANSPARENT);
            SelectObject(lpdis->hDC, dlg->hFontBold);
            DrawTextA(lpdis->hDC, text, -1, &lpdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_SEARCH_BTN:
            dlg->OnSearch();
            break;
        case ID_CLOSE_BTN:
            ShowWindow(hwnd, SW_HIDE);
            break;
        case ID_SEARCH_BOX:
            if (HIWORD(wParam) == EN_SETFOCUS) {
                dlg->EditFocused = true;
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                dlg->EditFocused = false;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case ID_LOAD_DECK_A:
            dlg->OnLoadToDeck(0);
            break;
        case ID_LOAD_DECK_B:
            dlg->OnLoadToDeck(1);
            break;
        case ID_ADD_AUTOMIX:
            dlg->OnLoadToDeck(2);
            break;
        case ID_ADD_SIDELIST:
            dlg->OnLoadToDeck(3);
            break;
        case ID_FORMAT_BTN: {
            // Toggle between MP3 and MP4
            dlg->CurrentFormat = (dlg->CurrentFormat == "mp4") ? "mp3" : "mp4";
            std::string label = (dlg->CurrentFormat == "mp4") ? "MP4" : "MP3";
            SetWindowTextA(dlg->hFormatBtn, label.c_str());
            InvalidateRect(dlg->hFormatBtn, NULL, TRUE);
            if (dlg->OnFormatChange) dlg->OnFormatChange(dlg->CurrentFormat);
            break;
        }
        }
        break;

    case WM_NOTIFY: {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->idFrom == ID_RESULTS_LIST) {
            if (nmhdr->code == NM_DBLCLK) {
                dlg->OnLoadToDeck(0);
            }
            else if (nmhdr->code == NM_RCLICK) {
                // Right-click context menu on search results
                int sel = ListView_GetNextItem(dlg->hResultsList, -1, LVNI_SELECTED);
                if (sel >= 0 && sel < (int)dlg->Results.size()) {
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU hMenu = CreatePopupMenu();
                    // Stream submenu (fast, direct URL - no full download)
                    HMENU hStreamMenu = CreatePopupMenu();
                    AppendMenuA(hStreamMenu, MF_STRING, ID_CTX_STREAM_DECK_A,   "Stream to Deck A");
                    AppendMenuA(hStreamMenu, MF_STRING, ID_CTX_STREAM_DECK_B,   "Stream to Deck B");
                    AppendMenuA(hStreamMenu, MF_STRING, ID_CTX_STREAM_AUTOMIX,  "Stream to Automix");
                    AppendMenuA(hStreamMenu, MF_STRING, ID_CTX_STREAM_SIDELIST, "Stream to Sidelist");
                    // Load submenu (full download + cache)
                    HMENU hLoadMenu = CreatePopupMenu();
                    AppendMenuA(hLoadMenu, MF_STRING, ID_LOAD_DECK_A,  "Load to Deck A");
                    AppendMenuA(hLoadMenu, MF_STRING, ID_LOAD_DECK_B,  "Load to Deck B");
                    AppendMenuA(hLoadMenu, MF_STRING, ID_ADD_AUTOMIX,  "Add to Automix");
                    AppendMenuA(hLoadMenu, MF_STRING, ID_ADD_SIDELIST, "Add to Sidelist");
                    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hStreamMenu, L"\u25BA Stream (instant)");
                    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hLoadMenu,   L"\u25BA Download & Load");
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuA(hMenu, MF_STRING, ID_CTX_DOWNLOAD_VIDEO, "Download Video (MP4)");
                    AppendMenuA(hMenu, MF_STRING, ID_CTX_DOWNLOAD_MP3,  "Download Audio (MP3)");
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hStreamMenu);
                    DestroyMenu(hLoadMenu);
                    DestroyMenu(hMenu);
                    const SearchResult& selResult = dlg->Results[sel];
                    if (cmd == ID_CTX_STREAM_DECK_A || cmd == ID_CTX_STREAM_DECK_B ||
                        cmd == ID_CTX_STREAM_AUTOMIX || cmd == ID_CTX_STREAM_SIDELIST) {
                        int streamDeck = (cmd == ID_CTX_STREAM_DECK_A) ? 0 :
                                         (cmd == ID_CTX_STREAM_DECK_B) ? 1 :
                                         (cmd == ID_CTX_STREAM_AUTOMIX) ? 2 : 3;
                        if (dlg->OnStreamTrack) {
                            dlg->SetStatusText("Streaming: " + selResult.title);
                            std::thread([dlg, selResult, streamDeck]() {
                                dlg->OnStreamTrack(selResult, streamDeck);
                            }).detach();
                        } else if (dlg->OnLoadTrack) {
                            dlg->SetStatusText("Loading: " + selResult.title);
                            std::thread([dlg, selResult, streamDeck]() {
                                dlg->OnLoadTrack(selResult, streamDeck);
                            }).detach();
                        }
                    } else if (cmd == ID_CTX_DOWNLOAD_VIDEO) {
                        if (dlg->OnDownloadVideo) {
                            dlg->SetStatusText("Downloading video: " + selResult.title);
                            dlg->OnDownloadVideo(selResult);
                        }
                    } else if (cmd == ID_CTX_DOWNLOAD_MP3) {
                        dlg->OnLoadToDeck(0);
                    } else if (cmd == ID_LOAD_DECK_A) {
                        dlg->OnLoadToDeck(0);
                    } else if (cmd == ID_LOAD_DECK_B) {
                        dlg->OnLoadToDeck(1);
                    } else if (cmd == ID_ADD_AUTOMIX) {
                        dlg->OnLoadToDeck(2);
                    } else if (cmd == ID_ADD_SIDELIST) {
                        dlg->OnLoadToDeck(3);
                    }
                }
            }
            else if (nmhdr->code == LVN_GETDISPINFOW) {
                // Virtual list data callback
                NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
                int idx = pdi->item.iItem;
                if (idx < 0 || idx >= (int)dlg->Results.size()) break;

                static wchar_t buf[512];
                if (pdi->item.mask & LVIF_TEXT) {
                    std::string text;
                    switch (pdi->item.iSubItem) {
                    case 0: {
                        std::string num = std::to_string(idx + 1);
                        bool hasMp3 = dlg->Results[idx].cachedMp3;
                        bool hasMp4 = dlg->Results[idx].cachedMp4;
                        if (hasMp3 && hasMp4) num += " \xE2\x99\xAA|\xE2\x96\xB6";  // music note | play arrow
                        else if (hasMp4) num += " \xE2\x96\xB6";  // play arrow for video
                        else if (hasMp3) num += " \xE2\x99\xAA";  // music note for audio
                        text = num;
                        break;
                    }
                    case 1:
                        text = dlg->Results[idx].title;
                        break;
                    case 2:
                        text = dlg->Results[idx].channel;
                        break;
                    case 3:
                        text = dlg->Results[idx].duration;
                        break;
                    }
                    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, buf, 512);
                    pdi->item.pszText = buf;
                }
            }
            else if (nmhdr->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                switch (lplvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    int item = (int)lplvcd->nmcd.dwItemSpec;
                    bool selected = (ListView_GetItemState(dlg->hResultsList, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;

                    bool hasMp3 = (item >= 0 && item < (int)dlg->Results.size()) ? dlg->Results[item].cachedMp3 : false;
                    bool hasMp4 = (item >= 0 && item < (int)dlg->Results.size()) ? dlg->Results[item].cachedMp4 : false;

                    COLORREF clrBack = selected ? CLR_ROW_SELECT :
                                       (item % 2 == 0) ? CLR_ROW_EVEN : CLR_ROW_ODD;

                    // Sub-item specific text colors
                    COLORREF clrText = CLR_TEXT_PRIMARY;
                    bool isCached = hasMp3 || hasMp4;
                    if (lplvcd->iSubItem == 0) {
                        // Row number column: blue for mp3 icon, purple for mp4 icon
                        if (hasMp3 && hasMp4) clrText = RGB(80, 160, 255);  // blue (first icon is music note)
                        else if (hasMp4) clrText = RGB(180, 100, 220);       // purple
                        else if (hasMp3) clrText = RGB(80, 160, 255);        // blue
                        else clrText = CLR_TEXT_DIM;
                    } else if (isCached) {
                        // Title, channel, duration: green for downloaded
                        clrText = RGB(80, 200, 120);
                    } else {
                        if (lplvcd->iSubItem == 2) clrText = CLR_TEXT_SECONDARY;
                        else if (lplvcd->iSubItem == 3) clrText = CLR_TEXT_SECONDARY;
                    }

                    lplvcd->clrText = clrText;
                    lplvcd->clrTextBk = clrBack;
                    return CDRF_DODEFAULT;
                }
                }
            }
            // Drag and drop initiation
            else if (nmhdr->code == LVN_BEGINDRAG) {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                dlg->BeginDragDrop(pnmv->iItem);
            }
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;

        if (hCtrl == dlg->hTitleLabel) {
            SetTextColor(hdc, CLR_TEXT_PRIMARY);
            SetBkColor(hdc, CLR_TITLEBAR);
            return (LRESULT)dlg->hTopBarBrush;
        }
        if (hCtrl == dlg->hStatusLabel) {
            SetTextColor(hdc, CLR_TEXT_SECONDARY);
            SetBkColor(hdc, CLR_BG_DARK);
            return (LRESULT)dlg->hBgBrush;
        }
        if (hCtrl == dlg->hProgressLabel) {
            SetTextColor(hdc, CLR_TEXT_SECONDARY);
            SetBkColor(hdc, CLR_BG_DARK);
            return (LRESULT)dlg->hBgBrush;
        }

        SetTextColor(hdc, CLR_TEXT_PRIMARY);
        SetBkColor(hdc, CLR_BG_DARK);
        return (LRESULT)dlg->hBgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, CLR_TEXT_PRIMARY);
        SetBkColor(hdc, CLR_SEARCH_BG);
        return (LRESULT)dlg->hEditBgBrush;
    }

    case WM_CTLCOLORBTN:
        return (LRESULT)dlg->hBgBrush;

    case WM_MOUSEMOVE: {
        static HWND hLastHover = NULL;
        POINT pt;
        GetCursorPos(&pt);
        HWND hChild = ChildWindowFromPoint(hwnd, pt);
        if (hChild != hLastHover) {
            if (hLastHover) InvalidateRect(hLastHover, NULL, FALSE);
            if (hChild) InvalidateRect(hChild, NULL, FALSE);
            hLastHover = hChild;
        }
        break;
    }

    case WM_SIZE:
        dlg->ResizeControls(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case 0x02E0: { // WM_DPICHANGED
        dlg->ScreenDPI = HIWORD(wParam);
        dlg->UpdateFonts();
        RECT* prcNew = (RECT*)lParam;
        SetWindowPos(hwnd, NULL, prcNew->left, prcNew->top,
            prcNew->right - prcNew->left, prcNew->bottom - prcNew->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    case WM_APP + 1:
        dlg->PopulateResults();
        break;

    case WM_USER + 100:
        // Update license status - called after window is fully initialized
        dlg->LogInfo("WM_USER+100 received - refreshing license status");
        dlg->RefreshLicenseStatus();
        break;

    case WM_LICENSE_STATUS_CHANGED:
        // License status changed - refresh the UI
        dlg->RefreshLicenseStatus();
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        dlg->hWnd = NULL;
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
// Search logic

void SearchDialog::OnSearch() {
    if (Searching) return;

    char query[512] = {};
    GetWindowTextA(hSearchBox, query, sizeof(query));
    if (strlen(query) == 0) return;

    // License check: require license for all searches
    if (!LicenseValid) {
        MessageBoxA(hWnd,
            "License required to search YouTube.\n\n"
            "Please activate your license to continue.",
            "YouTube Search - License Required",
            MB_OK | MB_ICONINFORMATION);
        
        // Trigger license dialog via callback if available
        if (OnLicenseRequired) OnLicenseRequired();
        return;
    }
    
    // Licensed user - Pro mode
    SetStatusText("Searching...");
    Searching = true;
    EnableWindow(hSearchBtn, FALSE);

    auto* data = new SearchThreadData{ this, std::string(query) };
    CreateThread(NULL, 0, SearchThreadProc, data, 0, NULL);
}

DWORD WINAPI SearchDialog::SearchThreadProc(LPVOID param) {
    auto* data = (SearchThreadData*)param;
    data->dialog->DoSearch(data->query);
    delete data;
    return 0;
}

void SearchDialog::DoSearch(const std::string& query) {
    Results.clear();

    std::string ytDlpPath = ToolsDir + "\\yt-dlp.exe";

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "YouTubeSource";
    std::filesystem::create_directories(tempDir);
    std::string outputFile = (tempDir / "search_results.txt").string();
    std::string batFile = (tempDir / "search.bat").string();

    {
        std::ofstream bat(batFile);
        bat << "@echo off" << std::endl;
        bat << "\"" << ytDlpPath << "\" --no-warnings --flat-playlist --print \"";
        bat << "%%(title)s|%%(duration_string)s|%%(uploader)s|%%(id)s|%%(webpage_url)s";
        bat << "\" \"ytsearch25:" << query << "\" > \"" << outputFile << "\" 2>&1" << std::endl;
        bat.close();
    }

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::string cmdLine = "cmd /c \"" + batFile + "\"";
    char* cmdLineBuf = _strdup(cmdLine.c_str());

    if (CreateProcessA(NULL, cmdLineBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 60000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    free(cmdLineBuf);

    std::ifstream inFile(outputFile);
    std::string line;
    while (std::getline(inFile, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) continue;
        if (line.find("WARNING:") == 0 || line.find("ERROR:") == 0) continue;

        SearchResult result;
        std::istringstream ss(line);
        std::string token;

        if (std::getline(ss, token, '|')) result.title = token;
        if (std::getline(ss, token, '|')) result.duration = token;
        if (std::getline(ss, token, '|')) result.channel = token;  // uploader for YouTube
        if (std::getline(ss, token, '|')) result.videoId = token;
        if (std::getline(ss, token, '|')) result.url = token;
        else result.url = "https://www.youtube.com/watch?v=" + result.videoId;

        if (!result.title.empty()) {
            // Check if this track is already cached (in library)
            if (!CacheDir.empty() && !result.videoId.empty()) {
                // Use videoId for cache filename (matches DownloadAndCacheTrack behavior)
                std::string mp3File = (std::filesystem::path(CacheDir) / (result.videoId + ".mp3")).string();
                std::string mp4File = (std::filesystem::path(CacheDir) / (result.videoId + ".mp4")).string();
                result.cachedMp3 = std::filesystem::exists(mp3File);
                result.cachedMp4 = std::filesystem::exists(mp4File);
            }
            Results.push_back(result);
        }
    }
    inFile.close();

    PostMessage(hWnd, WM_APP + 1, 0, 0);
}

void SearchDialog::PopulateResults() {
    // Virtual list mode: just set the item count
    ListView_SetItemCountEx(hResultsList, (int)Results.size(), LVSICF_NOINVALIDATEALL);
    InvalidateRect(hResultsList, NULL, TRUE);

    Searching = false;
    EnableWindow(hSearchBtn, TRUE);

    int mp3Count = 0, mp4Count = 0;
    for (const auto& r : Results) { if (r.cachedMp3) mp3Count++; if (r.cachedMp4) mp4Count++; }
    std::string statusMsg = std::to_string(Results.size()) + " results";
    if (mp3Count > 0 || mp4Count > 0) {
        statusMsg += "  |  ";
        if (mp3Count > 0) statusMsg += std::to_string(mp3Count) + " MP3";
        if (mp3Count > 0 && mp4Count > 0) statusMsg += ", ";
        if (mp4Count > 0) statusMsg += std::to_string(mp4Count) + " MP4";
        statusMsg += " in library";
    }
    SetStatusText(statusMsg);
}

void SearchDialog::OnLoadToDeck(int deck) {
    int sel = ListView_GetNextItem(hResultsList, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)Results.size()) {
        SetStatusText("Select a track first");
        return;
    }

    const SearchResult& result = Results[sel];
    std::string deckName;
    switch (deck) {
        case 0: deckName = "Deck A"; break;
        case 1: deckName = "Deck B"; break;
        case 2: deckName = "Automix"; break;
        case 3: deckName = "Sidelist"; break;
        default: deckName = "Deck A"; break;
    }
    SetStatusText("Loading to " + deckName + "...");

    // Show initial download progress
    DownloadProgress prog;
    prog.trackTitle = result.title;
    prog.phase = "Downloading";
    prog.percent = 0;
    prog.active = true;
    SetDownloadProgress(prog);

    if (OnLoadTrack) {
        SearchResult r = result;
        OnLoadTrack(r, deck);
    }
}

//////////////////////////////////////////////////////////////////////////
// Drag and Drop

void SearchDialog::BeginDragDrop(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= (int)Results.size()) return;

    const SearchResult& result = Results[itemIndex];
    std::string dragText = result.title + "\n" + result.url;

    // Create IDataObject for OLE drag and drop
    // Use simple text-based drag - VDJ can accept file drops
    // We'll use CF_TEXT with the YouTube URL

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dragText.size() + 1);
    if (!hMem) return;

    char* pMem = (char*)GlobalLock(hMem);
    if (pMem) {
        memcpy(pMem, dragText.c_str(), dragText.size() + 1);
        GlobalUnlock(hMem);
    }

    // For now, set drag cursor and let the user know
    SetStatusText("Drag to a VDJ deck to load...");

    // We use a simpler approach: initiate Windows DragDetect and on drop,
    // determine which deck based on drop position
    DragActive = true;
    DragItemIndex = itemIndex;

    // Use ImageList drag for visual feedback
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hResultsList, &pt);

    HIMAGELIST hDragImg = ListView_CreateDragImage(hResultsList, itemIndex, &pt);
    if (hDragImg) {
        ImageList_BeginDrag(hDragImg, 0, 0, 0);
        GetCursorPos(&pt);
        ImageList_DragEnter(NULL, pt.x, pt.y);

        SetCapture(hWnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_MOUSEMOVE) {
                POINT dragPt = { (short)LOWORD(msg.lParam), (short)HIWORD(msg.lParam) };
                ClientToScreen(hWnd, &dragPt);
                ImageList_DragMove(dragPt.x, dragPt.y);
            }
            else if (msg.message == WM_LBUTTONUP) {
                ImageList_DragLeave(NULL);
                ImageList_EndDrag();
                ReleaseCapture();

                // Check if dropped on Deck A or Deck B button
                POINT dropPt = { (short)LOWORD(msg.lParam), (short)HIWORD(msg.lParam) };
                HWND hDropTarget = ChildWindowFromPoint(hWnd, dropPt);

                if (hDropTarget == hLoadDeckA) {
                    OnLoadToDeck(0);
                } else if (hDropTarget == hLoadDeckB) {
                    OnLoadToDeck(1);
                } else {
                    SetStatusText("Drop on DECK A or DECK B to load");
                }
                break;
            }
            else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                ImageList_DragLeave(NULL);
                ImageList_EndDrag();
                ReleaseCapture();
                SetStatusText("Drag cancelled");
                break;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        ImageList_Destroy(hDragImg);
    }

    DragActive = false;
    DragItemIndex = -1;
    GlobalFree(hMem);
}

//////////////////////////////////////////////////////////////////////////
// Logging System for SearchDialog

void SearchDialog::InitializeLogging()
{
    try {
        // Get temp directory for log file
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        LogFilePath = std::string(tempPath) + "YouTubeSearchDialog.log";
        
        // Create new log file (truncate existing)
        std::ofstream logFile(LogFilePath, std::ios::trunc);
        if (logFile.is_open()) {
            logFile.close();
            LogInfo("=== YouTube Search Dialog Log Started ===");
        }
    } catch (...) {
        // Silently fail if logging can't be initialized
    }
}

void SearchDialog::Log(const std::string& level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(LogMutex);
    try {
        std::ofstream logFile(LogFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << GetTimestamp() << " [" << level << "] " << message << std::endl;
        }
    } catch (...) {}
}

void SearchDialog::LogDebug(const std::string& message) { Log("DEBUG", message); }
void SearchDialog::LogInfo(const std::string& message) { Log("INFO", message); }
void SearchDialog::LogWarning(const std::string& message) { Log("WARN", message); }
void SearchDialog::LogError(const std::string& message) { Log("ERROR", message); }

std::string SearchDialog::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
#else
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
#endif
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}
