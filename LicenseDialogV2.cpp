#define _CRT_SECURE_NO_WARNINGS
#include "LicenseDialogV2.h"
#include <commctrl.h>
#include <gdiplus.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <shlwapi.h>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")

// Custom Windows message for license status updates
#define WM_LICENSE_STATUS_CHANGED (WM_USER + 200)

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

LicenseDialogV2::LicenseDialogV2()
{
    // Initialize logging first
    try {
        InitializeLogging();
    } catch (...) {
        // Continue without logging if it fails
    }
    
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = GdiplusStartup(&GdiplusToken, &gdiplusStartupInput, NULL);
    if (status != Gdiplus::Ok) {
        LogError("Failed to initialize GDI+ in LicenseDialogV2 constructor");
        MessageBoxA(NULL, "Critical Error: Failed to initialize graphics system (GDI+). Please ensure Windows is up to date.", 
            "YouTube Source - Error", MB_OK | MB_ICONERROR);
        GdiplusToken = 0;
    } else {
        LogInfo("GDI+ initialized successfully in LicenseDialogV2");
    }
    
    // Create fonts
    HDC hdc = GetDC(NULL);
    int titleSize = -MulDiv(24, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    int normalSize = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    int boldSize = -MulDiv(11, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    
    hTitleFont = CreateFontA(titleSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    hNormalFont = CreateFontA(normalSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    hBoldFont = CreateFontA(boldSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    hOrangeBrush = CreateSolidBrush(COLOR_ORANGE);
    hDarkBrush = CreateSolidBrush(COLOR_DARK);
    
    // Initialize logging first so we can log logo loading
    InitializeLogging();
    
    // Load YouTube logo from DLL directory
    char dllPath[MAX_PATH];
    HMODULE hModule = NULL;
    
    // Get the DLL module handle (try both 32-bit and 64-bit names)
    hModule = GetModuleHandleA("YouTubeSource64.dll");
    if (!hModule) hModule = GetModuleHandleA("YouTubeSource32.dll");
    if (!hModule) hModule = GetModuleHandleA("YouTubeSource.dll");
    
    if (hModule) {
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);
        PathRemoveFileSpecA(dllPath);
        LogoPath = std::string(dllPath) + "\\Youtube.png";
        LogInfo("Logo path (from DLL): " + LogoPath);
    } else {
        // Fallback to executable directory if DLL handle not found
        GetModuleFileNameA(NULL, dllPath, MAX_PATH);
        PathRemoveFileSpecA(dllPath);
        LogoPath = std::string(dllPath) + "\\Youtube.png";
        LogInfo("Logo path (from EXE fallback): " + LogoPath);
    }
    
    // Load logo with GDI+
    std::wstring wLogoPath(LogoPath.begin(), LogoPath.end());
    pLogoImage = Gdiplus::Image::FromFile(wLogoPath.c_str());
    if (!pLogoImage || pLogoImage->GetLastStatus() != Gdiplus::Ok) {
        LogWarning("Failed to load YouTube logo from: " + LogoPath);
        if (pLogoImage) {
            delete pLogoImage;
            pLogoImage = nullptr;
        }
    } else {
        LogInfo("YouTube logo loaded successfully from: " + LogoPath);
    }
}

LicenseDialogV2::~LicenseDialogV2()
{
    if (hTitleFont) DeleteObject(hTitleFont);
    if (hNormalFont) DeleteObject(hNormalFont);
    if (hBoldFont) DeleteObject(hBoldFont);
    if (hOrangeBrush) DeleteObject(hOrangeBrush);
    if (hDarkBrush) DeleteObject(hDarkBrush);
    if (hLogoBitmap) DeleteObject(hLogoBitmap);
    if (pLogoImage) delete pLogoImage;
    
    // Shut down GDI+
    if (GdiplusToken) {
        GdiplusShutdown(GdiplusToken);
        GdiplusToken = 0;
        LogInfo("GDI+ shut down successfully in LicenseDialogV2 destructor");
    }
}

void LicenseDialogV2::SetLicenseInfo(const std::string& key, const std::string& expiry, bool isLicensed)
{
    LicenseKey = key;
    ExpiryDate = expiry;
    IsLicensed = isLicensed;
}

void LicenseDialogV2::SetMachineID(const std::string& machineId)
{
    MachineID = machineId;
}

void LicenseDialogV2::SetMarketplaceAccount(bool loggedIn, const std::string& email, int currentActivations, int maxActivations)
{
    IsLoggedIn = loggedIn;
    AccountEmail = email;
    CurrentActivations = currentActivations;
    MaxActivations = maxActivations;
}

void LicenseDialogV2::SetTrialInfo(int searchesUsed, int maxSearches)
{
    SearchesUsed = searchesUsed;
    MaxSearches = maxSearches;
}

void LicenseDialogV2::SetSavedEmail(const std::string& email)
{
    SavedEmail = email;
}

void LicenseDialogV2::SetSavedPassword(const std::string& password)
{
    SavedPassword = password;
}

void LicenseDialogV2::SetSavePasswordEnabled(bool enabled)
{
    SavePasswordEnabled = enabled;
}

void LicenseDialogV2::SetLicenseType(const std::string& type)
{
    LicenseType = type;
}

void LicenseDialogV2::SetTokenExpiry(const std::string& expiry)
{
    TokenExpiry = expiry;
}

void LicenseDialogV2::SetActivations(int current, int max)
{
    CurrentActivations = current;
    MaxActivations = max;
}

void LicenseDialogV2::SetSubLicenses(int current, int max)
{
    CurrentSubLicenses = current;
    MaxSubLicenses = max;
}

void LicenseDialogV2::SetSharedLicenseInfo(bool isShared, const std::string& ownerName, 
                                           const std::string& ownerEmail, const std::string& sharedDate)
{
    IsSharedLicense = isShared;
    SharedOwnerName = ownerName;
    SharedOwnerEmail = ownerEmail;
    SharedDate = sharedDate;
}

void LicenseDialogV2::RefreshLicenseDisplay()
{
    LogInfo("RefreshLicenseDisplay called");
    if (!hDialog || !IsWindow(hDialog)) {
        LogWarning("Cannot refresh - dialog window is invalid");
        return;
    }
    
    // Destroy all child windows (controls)
    HWND hChild = GetWindow(hDialog, GW_CHILD);
    while (hChild) {
        HWND hNext = GetWindow(hChild, GW_HWNDNEXT);
        DestroyWindow(hChild);
        hChild = hNext;
    }
    
    // Recreate the dialog controls with updated data
    OnInitDialog(hDialog);
    
    // Force repaint
    InvalidateRect(hDialog, NULL, TRUE);
    UpdateWindow(hDialog);
}

int LicenseDialogV2::ShowDialog(HWND parent)
{
    LogInfo("ShowDialog called");
    
    // Check if GDI+ was initialized successfully
    if (GdiplusToken == 0) {
        LogError("Cannot show dialog - GDI+ not initialized");
        MessageBoxA(NULL, "Failed to initialize graphics system. Please contact support.", 
            "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Initialize COM for this thread (required for some Windows operations)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        LogWarning("COM initialization failed with HRESULT: " + std::to_string(hr));
    } else {
        LogInfo("COM initialized successfully");
    }
    
    // Register window class
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
    wc.lpfnWndProc = DialogProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "LicenseDialogV2Class";
    wc.hbrBackground = hDarkBrush;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&wc);
    
    // Create window
    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST | WS_EX_APPWINDOW,
        "LicenseDialogV2Class",
        "YouTube Source - License Manager",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        parent, NULL, GetModuleHandleA(NULL), this);
    
    if (!hwnd) {
        DWORD error = GetLastError();
        LogError("Failed to create license dialog window. Error code: " + std::to_string(error));
        MessageBoxA(NULL, ("Failed to create license dialog window. Error code: " + std::to_string(error)).c_str(),
            "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    LogInfo("License dialog window created successfully");
    
    // Center window on primary monitor
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - (rc.right - rc.left)) / 2;
    int y = (screenHeight - (rc.bottom - rc.top)) / 2;
    
    // Ensure window is visible (not off-screen)
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    
    LogInfo("Positioning window at: " + std::to_string(x) + ", " + std::to_string(y) + 
            " (screen: " + std::to_string(screenWidth) + "x" + std::to_string(screenHeight) + ")");
    
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    
    // Force window to be visible and on top
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    
    LogInfo("Window visibility enforced");
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT || !IsWindow(hwnd)) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Uninitialize COM
    CoUninitialize();
    
    return (int)msg.wParam;
}

INT_PTR CALLBACK LicenseDialogV2::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LicenseDialogV2* pThis = NULL;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (LicenseDialogV2*)pCreate->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->hDialog = hwnd;
    }
    
    pThis = (LicenseDialogV2*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (!pThis) return DefWindowProcA(hwnd, msg, wParam, lParam);
    
    if (msg == WM_CREATE) {
        pThis->OnInitDialog(hwnd);
        return 0;
    }
    
    switch (msg) {
        case WM_PAINT:
            pThis->OnPaint(hwnd);
            return TRUE;
            
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, pThis->COLOR_TEXT);
            return (INT_PTR)pThis->hDarkBrush;
            
        case WM_CTLCOLOREDIT:
            SetBkColor((HDC)wParam, pThis->COLOR_DARKER);
            SetTextColor((HDC)wParam, pThis->COLOR_TEXT);
            return (INT_PTR)CreateSolidBrush(pThis->COLOR_DARKER);
            
        case WM_COMMAND:
            pThis->OnCommand(hwnd, wParam);
            return TRUE;
            
        case WM_LICENSE_STATUS_CHANGED:
            pThis->RefreshLicenseDisplay();
            return TRUE;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return TRUE;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
    }
    
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void LicenseDialogV2::OnInitDialog(HWND hwnd)
{
    // Set window size and center - increased height for enhanced info
    int width = 700;
    int height = IsSharedLicense ? 850 : 750;
    
    RECT screenRect;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &screenRect, 0);
    int x = (screenRect.right - width) / 2;
    int y = (screenRect.bottom - height) / 2;
    
    SetWindowPos(hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    SetWindowTextA(hwnd, "YouTube Source - License Manager");
    LogInfo("License Manager dialog initialized");
    
    int yPos = 140;
    int leftMargin = 40;
    int controlWidth = 620;
    
    // Machine ID section
    CreateWindowExA(0, "STATIC", "Machine ID:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, 120, 20, hwnd, NULL, NULL, NULL);
    
    CreateWindowExA(WS_EX_CLIENTEDGE, "STATIC", MachineID.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin + 130, yPos, 280, 25, hwnd, NULL, NULL, NULL);
    
    CreateWindowExA(0, "BUTTON", "Copy",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin + 420, yPos, 80, 25, hwnd, (HMENU)IDC_COPY_MACHINEID_BTN, NULL, NULL);
    
    yPos += 50;
    
    // Enhanced Status Section Header
    CreateWindowExA(0, "STATIC", "----------------------------------------------------",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, controlWidth, 20, hwnd, NULL, NULL, NULL);
    yPos += 25;
    
    HWND hStatusHeader = CreateWindowExA(0, "STATIC", "Status",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, 200, 25, hwnd, NULL, NULL, NULL);
    SendMessageA(hStatusHeader, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
    yPos += 30;
    
    // Multi-column status layout
    int col1 = leftMargin;
    int col2 = leftMargin + 120;
    int col3 = leftMargin + 200;
    int col4 = leftMargin + 310;
    
    // Row 1: License Status | Owner Email (if sub-license)
    CreateWindowExA(0, "STATIC", "License Status:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col1, yPos, 110, 20, hwnd, NULL, NULL, NULL);
    
    std::string statusText = IsLicensed ? "Active" : "Unlicensed";
    CreateWindowExA(0, "STATIC", statusText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col2, yPos, 75, 20, hwnd, NULL, NULL, NULL);
    
    // Show Owner Email if sub-license
    if (IsSharedLicense && !SharedOwnerEmail.empty()) {
        CreateWindowExA(0, "STATIC", "Owner Email:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col3, yPos, 95, 20, hwnd, NULL, NULL, NULL);
        CreateWindowExA(0, "STATIC", SharedOwnerEmail.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col4, yPos, 180, 20, hwnd, NULL, NULL, NULL);
    }
    yPos += 25;
    
    // Row 2: Expires | License Type
    CreateWindowExA(0, "STATIC", "Expires:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col1, yPos, 110, 20, hwnd, NULL, NULL, NULL);
    
    std::string expiryText = "N/A";
    if (IsLicensed) {
        if (!LicenseType.empty() && (LicenseType == "lifetime" || LicenseType == "Lifetime")) {
            expiryText = "Unlimited";
        } else if (!ExpiryDate.empty()) {
            expiryText = ExpiryDate;
        }
    }
    CreateWindowExA(0, "STATIC", expiryText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col2, yPos, 75, 20, hwnd, NULL, NULL, NULL);
    
    CreateWindowExA(0, "STATIC", "License Type:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col3, yPos, 100, 20, hwnd, NULL, NULL, NULL);
    
    std::string typeText = "N/A";
    if (IsLicensed && !LicenseType.empty()) {
        // Normalize license type display
        std::string lowerType = LicenseType;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
        
        if (lowerType == "monthly") {
            typeText = "Monthly";
        } else if (lowerType == "yearly" || lowerType == "annual") {
            typeText = "Yearly";
        } else if (lowerType == "lifetime") {
            typeText = "Lifetime";
        } else if (lowerType.find("day") != std::string::npos) {
            typeText = "Custom Days";
        } else {
            // Capitalize first letter of whatever type it is
            typeText = LicenseType;
            if (!typeText.empty()) {
                typeText[0] = toupper(typeText[0]);
            }
        }
    }
    CreateWindowExA(0, "STATIC", typeText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col4, yPos, 100, 20, hwnd, NULL, NULL, NULL);
    yPos += 25;
    
    // Row 3: Activations | Session Expires
    CreateWindowExA(0, "STATIC", "Activations:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col1, yPos, 110, 20, hwnd, NULL, NULL, NULL);
    
    std::string activText = "N/A";
    if (IsLicensed && MaxActivations > 0) {
        std::ostringstream activOss;
        activOss << CurrentActivations << " / " << MaxActivations;
        activText = activOss.str();
    }
    CreateWindowExA(0, "STATIC", activText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        col2, yPos, 75, 20, hwnd, NULL, NULL, NULL);
    
    if (IsLicensed && !TokenExpiry.empty()) {
        CreateWindowExA(0, "STATIC", "Session Expires:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col3, yPos, 105, 20, hwnd, NULL, NULL, NULL);
        CreateWindowExA(0, "STATIC", TokenExpiry.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col4, yPos, 150, 20, hwnd, NULL, NULL, NULL);
    }
    yPos += 25;
    
    // Row 4: Sub-Licenses (only show if user has sub-license capability and is licensed)
    if (IsLicensed && MaxSubLicenses > 0) {
        CreateWindowExA(0, "STATIC", "Sub-Licenses:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col1, yPos, 110, 20, hwnd, NULL, NULL, NULL);
        
        std::ostringstream subOss;
        subOss << CurrentSubLicenses << " / " << MaxSubLicenses;
        CreateWindowExA(0, "STATIC", subOss.str().c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            col2, yPos, 75, 20, hwnd, NULL, NULL, NULL);
        yPos += 25;
    }
    
    yPos += 15;
    
    // Sub-License Information Section (only shown if license is shared)
    if (IsSharedLicense && !SharedOwnerName.empty()) {
        // Draw orange gradient box with rounded corners
        HDC hdc = GetDC(hwnd);
        int boxX = leftMargin - 5;
        int boxY = yPos - 5;
        int boxW = controlWidth + 10;
        int boxH = 95;
        
        // Create rounded rectangle region
        HRGN hRgn = CreateRoundRectRgn(boxX, boxY, boxX + boxW, boxY + boxH, 8, 8);
        
        // Fill with orange gradient (lighter at top, darker at bottom)
        RECT gradRect = {boxX, boxY, boxX + boxW, boxY + boxH};
        TRIVERTEX vertex[2];
        vertex[0].x = gradRect.left;
        vertex[0].y = gradRect.top;
        vertex[0].Red = 0xFF00;    // RGB(255, 160, 0) - lighter orange
        vertex[0].Green = 0xA000;
        vertex[0].Blue = 0x0000;
        vertex[0].Alpha = 0x0000;
        
        vertex[1].x = gradRect.right;
        vertex[1].y = gradRect.bottom;
        vertex[1].Red = 0xFF00;    // RGB(255, 120, 0) - darker orange
        vertex[1].Green = 0x7800;
        vertex[1].Blue = 0x0000;
        vertex[1].Alpha = 0x0000;
        
        GRADIENT_RECT gRect = {0, 1};
        SelectClipRgn(hdc, hRgn);
        GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
        SelectClipRgn(hdc, nullptr);
        
        // Draw subtle border
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(200, 100, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, boxX, boxY, boxX + boxW, boxY + boxH, 8, 8);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBorderPen);
        DeleteObject(hRgn);
        ReleaseDC(hwnd, hdc);
        
        // Section header
        HWND hSharedHeader = CreateWindowExA(0, "STATIC", "Sub-License Information",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 5, yPos + 2, 300, 20, hwnd, NULL, NULL, NULL);
        SendMessageA(hSharedHeader, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
        yPos += 28;
        
        // Row 1: Sub Account Name (extract from email)
        CreateWindowExA(0, "STATIC", "Sub Account Name:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 10, yPos, 130, 20, hwnd, NULL, NULL, NULL);
        
        std::string subUserName = AccountEmail;
        size_t atPos = subUserName.find('@');
        if (atPos != std::string::npos) {
            subUserName = subUserName.substr(0, atPos);
        }
        CreateWindowExA(0, "STATIC", subUserName.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 145, yPos, 350, 20, hwnd, NULL, NULL, NULL);
        yPos += 22;
        
        // Row 2: Sub Account Email
        CreateWindowExA(0, "STATIC", "Sub Account Email:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 10, yPos, 130, 20, hwnd, NULL, NULL, NULL);
        CreateWindowExA(0, "STATIC", AccountEmail.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 145, yPos, 350, 20, hwnd, NULL, NULL, NULL);
        yPos += 22;
        
        // Row 3: Shared On date
        if (!SharedDate.empty()) {
            CreateWindowExA(0, "STATIC", "Shared On:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                leftMargin + 10, yPos, 90, 20, hwnd, NULL, NULL, NULL);
            CreateWindowExA(0, "STATIC", SharedDate.c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                leftMargin + 100, yPos, 350, 20, hwnd, NULL, NULL, NULL);
            yPos += 22;
        }
        
        yPos += 20; // Extra spacing after orange box
        LogInfo("Sub-License Information box displayed");
    }
    
    // Marketplace account section
    CreateWindowExA(0, "STATIC", "----------------------------------------------------",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, controlWidth, 20, hwnd, NULL, NULL, NULL);
    yPos += 30;
    
    CreateWindowExA(0, "STATIC", "Marketplace Account",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, 520, 25, hwnd, NULL, NULL, NULL);
    yPos += 35;
    
    if (IsLoggedIn) {
        // Show logged in state
        CreateWindowExA(0, "STATIC", ("Logged in as: " + AccountEmail).c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 20, yPos, 500, 20, hwnd, NULL, NULL, NULL);
        yPos += 30;
        
        std::ostringstream devOss;
        devOss << "Activations: " << CurrentActivations << " / " << MaxActivations;
        CreateWindowExA(0, "STATIC", devOss.str().c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin + 20, yPos, 500, 20, hwnd, NULL, NULL, NULL);
        yPos += 40;
        
        // Logout and Dashboard buttons
        CreateWindowExA(0, "BUTTON", "Logout",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            leftMargin, yPos, 120, 35, hwnd, (HMENU)IDC_LOGOUT_BTN, NULL, NULL);
        
        CreateWindowExA(0, "BUTTON", "Open Dashboard",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            leftMargin + 140, yPos, 160, 35, hwnd, (HMENU)IDC_DASHBOARD_BTN, NULL, NULL);
    } else {
        // Show login form
        CreateWindowExA(0, "STATIC", "Email:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin, yPos, 100, 20, hwnd, NULL, NULL, NULL);
        yPos += 25;
        
        HWND hEmailEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", SavedEmail.c_str(),
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            leftMargin, yPos, controlWidth, 30, hwnd, (HMENU)IDC_EMAIL_EDIT, NULL, NULL);
        yPos += 45;
        
        CreateWindowExA(0, "STATIC", "Password:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            leftMargin, yPos, 100, 20, hwnd, NULL, NULL, NULL);
        yPos += 25;
        
        HWND hPasswordEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", SavedPassword.c_str(),
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_PASSWORD | ES_AUTOHSCROLL,
            leftMargin, yPos, controlWidth, 30, hwnd, (HMENU)IDC_PASSWORD_EDIT, NULL, NULL);
        yPos += 40;
        
        // Save password checkbox
        HWND hSavePasswordCheck = CreateWindowExA(0, "BUTTON", "Remember password",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            leftMargin, yPos, 200, 25, hwnd, (HMENU)IDC_SAVE_PASSWORD_CHECK, NULL, NULL);
        if (SavePasswordEnabled) {
            SendMessage(hSavePasswordCheck, BM_SETCHECK, BST_CHECKED, 0);
        }
        yPos += 35;
        
        CreateWindowExA(0, "BUTTON", "Login to Marketplace",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            leftMargin, yPos, 200, 35, hwnd, (HMENU)IDC_LOGIN_BTN, NULL, NULL);
    }
    
    // Support email at bottom
    yPos = height - 110;
    CreateWindowExA(0, "STATIC", "Support: support@djeventsuite.cloud",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, yPos, 400, 20, hwnd, NULL, NULL, NULL);
    
    // Close button at bottom
    CreateWindowExA(0, "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        width - 140, height - 80, 100, 35, hwnd, (HMENU)IDC_CLOSE_BTN, NULL, NULL);
    
    // Apply fonts to all controls
    EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
        LicenseDialogV2* pThis = (LicenseDialogV2*)lParam;
        SendMessageA(hwndChild, WM_SETFONT, (WPARAM)pThis->hNormalFont, TRUE);
        return TRUE;
    }, (LPARAM)this);
}

void LicenseDialogV2::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // Draw gradient background
    DrawGradientBackground(hdc, rect);
    
    // Draw header section (increased height for better spacing)
    RECT headerRect = {0, 0, rect.right, 120};
    FillRect(hdc, &headerRect, hDarkBrush);
    
    // Draw YouTube logo if available
    int logoX = 20;
    int logoY = 20;
    int logoSize = 65;
    int textX = logoX + logoSize + 20;
    
    if (pLogoImage) {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(pLogoImage, logoX, logoY, logoSize, logoSize);
    }
    
    // Draw title text in YouTube red
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 0, 0));  // YouTube red
    SelectObject(hdc, hTitleFont);
    
    RECT titleRect = {textX, 35, rect.right - 20, 75};
    DrawTextA(hdc, "YouTube Source", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    SetTextColor(hdc, COLOR_TEXT);
    SelectObject(hdc, hNormalFont);
    RECT subtitleRect = {textX, 75, rect.right - 20, 105};
    DrawTextA(hdc, "Marketplace License Manager", -1, &subtitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    EndPaint(hwnd, &ps);
}

void LicenseDialogV2::DrawGradientBackground(HDC hdc, RECT rect)
{
    TRIVERTEX vertex[2];
    vertex[0].x = 0;
    vertex[0].y = 0;
    vertex[0].Red = GetRValue(COLOR_DARKER) << 8;
    vertex[0].Green = GetGValue(COLOR_DARKER) << 8;
    vertex[0].Blue = GetBValue(COLOR_DARKER) << 8;
    vertex[0].Alpha = 0x0000;
    
    vertex[1].x = rect.right;
    vertex[1].y = rect.bottom;
    vertex[1].Red = GetRValue(COLOR_DARK) << 8;
    vertex[1].Green = GetGValue(COLOR_DARK) << 8;
    vertex[1].Blue = GetBValue(COLOR_DARK) << 8;
    vertex[1].Alpha = 0x0000;
    
    GRADIENT_RECT gRect = {0, 1};
    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

void LicenseDialogV2::OnCommand(HWND hwnd, WPARAM wParam)
{
    int id = LOWORD(wParam);
    
    switch (id) {
        case IDC_LOGIN_BTN: {
            LogInfo("Login button clicked");
            char email[256] = {0};
            char password[256] = {0};
            GetDlgItemTextA(hwnd, IDC_EMAIL_EDIT, email, sizeof(email));
            GetDlgItemTextA(hwnd, IDC_PASSWORD_EDIT, password, sizeof(password));
            
            if (strlen(email) == 0 || strlen(password) == 0) {
                LogWarning("Login attempted with empty email or password");
                MessageBoxA(hwnd, "Please enter both email and password.", "Login", MB_OK | MB_ICONWARNING);
                break;
            }
            
            // Check if user wants to save password
            HWND hCheckbox = GetDlgItem(hwnd, IDC_SAVE_PASSWORD_CHECK);
            SavePasswordEnabled = (SendMessage(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
            
            if (OnLogin) {
                LogInfo("Attempting login for: " + std::string(email));
                bool loginSuccess = OnLogin(email, password, SavePasswordEnabled);
                if (loginSuccess) {
                    LogInfo("Login successful for: " + std::string(email));
                    MessageBoxA(hwnd, "Login successful! Please restart the dialog to see updated information.",
                        "Success", MB_OK | MB_ICONINFORMATION);
                    DestroyWindow(hwnd);
                } else {
                    LogWarning("Login failed for: " + std::string(email));
                    // Show error with option to purchase license
                    int result = MessageBoxA(hwnd,
                        "Login failed - No active license found for this account.\n\n"
                        "To use YouTube Source, you need to purchase a license.\n\n"
                        "Would you like to visit the marketplace to purchase a license?",
                        "License Required",
                        MB_YESNO | MB_ICONINFORMATION);
                    
                    if (result == IDYES && OnDashboard) {
                        OnDashboard(); // Opens marketplace
                    }
                }
            }
            break;
        }
        
        case IDC_LOGOUT_BTN:
            LogInfo("Logout button clicked");
            if (OnLogout) {
                if (MessageBoxA(hwnd, "Are you sure you want to logout?", "Logout",
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    LogInfo("User confirmed logout");
                    OnLogout();
                    MessageBoxA(hwnd, "Logged out successfully.", "Logout", MB_OK | MB_ICONINFORMATION);
                    DestroyWindow(hwnd);
                }
            }
            break;
            
        case IDC_DASHBOARD_BTN:
            LogInfo("Dashboard button clicked");
            if (OnDashboard) {
                OnDashboard();
            }
            break;
            
        case IDC_COPY_MACHINEID_BTN:
            LogInfo("Copy Machine ID button clicked");
            if (OpenClipboard(hwnd)) {
                EmptyClipboard();
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, MachineID.length() + 1);
                if (hMem) {
                    memcpy(GlobalLock(hMem), MachineID.c_str(), MachineID.length() + 1);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_TEXT, hMem);
                }
                CloseClipboard();
                LogInfo("Machine ID copied to clipboard: " + MachineID);
                MessageBoxA(hwnd, "Machine ID copied to clipboard!", "Copied", MB_OK | MB_ICONINFORMATION);
            }
            break;
            
        case IDC_CLOSE_BTN:
            LogInfo("Close button clicked - destroying dialog");
            DestroyWindow(hwnd);
            break;
    }
}

//////////////////////////////////////////////////////////////////////////
// Logging System for LicenseDialogV2

void LicenseDialogV2::InitializeLogging()
{
    try {
        // Get temp directory for log file
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath)) {
            LogFilePath = std::string(tempPath) + "YouTubeLicenseManager.log";
            
            // Create new log file (truncate existing)
            std::ofstream logFile(LogFilePath, std::ios::trunc);
            if (logFile.is_open()) {
                logFile.close();
                LogInfo("=== YouTube License Manager Log Started ===");
            }
        }
    } catch (...) {
        // Silently fail if logging can't be initialized
    }
}

void LicenseDialogV2::Log(const std::string& level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(LogMutex);
    try {
        std::ofstream logFile(LogFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << GetTimestamp() << " [" << level << "] " << message << std::endl;
        }
    } catch (...) {}
}

void LicenseDialogV2::LogDebug(const std::string& message) { Log("DEBUG", message); }
void LicenseDialogV2::LogInfo(const std::string& message) { Log("INFO", message); }
void LicenseDialogV2::LogWarning(const std::string& message) { Log("WARN", message); }
void LicenseDialogV2::LogError(const std::string& message) { Log("ERROR", message); }

std::string LicenseDialogV2::GetTimestamp()
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
