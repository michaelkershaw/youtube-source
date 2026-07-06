#include "LicenseDialog.h"
#include <windows.h>
#include <commctrl.h>
#include "PluginVersion.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

#ifndef EM_SETCUEBANNER
#define EM_SETCUEBANNER 0x1501
#endif

// Dark theme colors
#define CLR_BG          RGB(15,  16,  23)
#define CLR_PANEL       RGB(26,  27,  38)
#define CLR_BORDER      RGB(42,  42,  58)
#define CLR_TEXT        RGB(220, 220, 220)
#define CLR_TEXT_DIM    RGB(140, 140, 160)
#define CLR_GREEN       RGB(80,  200, 120)
#define CLR_RED         RGB(240, 80,  80)
#define CLR_ORANGE      RGB(255, 160, 50)
#define CLR_BLUE        RGB(80,  160, 255)
#define CLR_BTN_ACT     RGB(80,  200, 120)
#define CLR_BTN_REM     RGB(200, 60,  60)
#define CLR_BTN_CLOSE   RGB(60,  60,  80)
#define CLR_BTN_COPY    RGB(50,  100, 180)

static LicenseDialog* g_pLicDlg = nullptr;

LicenseDialog::LicenseDialog()
{
    hBgBrush    = CreateSolidBrush(CLR_BG);
    hPanelBrush = CreateSolidBrush(CLR_PANEL);
    hFontBig  = CreateFontA(20, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    hFontMed  = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    hFontMono = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Courier New");
}

LicenseDialog::~LicenseDialog()
{
    if (hBgBrush)    { DeleteObject(hBgBrush);    hBgBrush    = NULL; }
    if (hPanelBrush) { DeleteObject(hPanelBrush); hPanelBrush = NULL; }
    if (hFontBig)    { DeleteObject(hFontBig);    hFontBig    = NULL; }
    if (hFontMed)    { DeleteObject(hFontMed);    hFontMed    = NULL; }
    if (hFontMono)   { DeleteObject(hFontMono);   hFontMono   = NULL; }
}

void LicenseDialog::SetInfo(const LicenseInfo& info)
{
    Info = info;
    if (hDlg) UpdateDisplay(hDlg);
}

void LicenseDialog::Show(HINSTANCE hInst, HWND hParent)
{
    hInstance = hInst;

    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc   = DlgProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "YTLicenseDlg";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    g_pLicDlg = this;

    std::string title = "SoundCloud Source - License Manager v" PLUGIN_VERSION;
    
    // Get DPI for scaling - use effective DPI for better 4K support
    HDC hdc = GetDC(NULL);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
    ReleaseDC(NULL, hdc);
    
    // For 4K monitors, ensure minimum scaling
    if (dpi < 120) dpi = 120;  // Minimum 125% scaling
    auto Scale = [dpi](int val) { return MulDiv(val, dpi, 96); };
    
    int baseHeight = Info.updateAvailable ? 540 : 480;
    hDlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "YTLicenseDlg", title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, Scale(680), Scale(baseHeight),
        hParent, NULL, hInst, NULL);

    if (!hDlg) return;

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    g_pLicDlg = nullptr;
}

void LicenseDialog::CreateControls(HWND hwnd)
{
    // Get DPI for scaling
    HDC hdc = GetDC(hwnd);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
    ReleaseDC(hwnd, hdc);
    
    // For 4K monitors, ensure minimum scaling
    if (dpi < 120) dpi = 120;  // Minimum 125% scaling
    auto Scale = [dpi](int val) { return MulDiv(val, dpi, 96); };
    
    int x = Scale(24), w = Scale(632), y = Scale(14);

    // Title label (owner-drawn via WM_CTLCOLORSTATIC)
    CreateWindowA("STATIC", "SoundCloud Source - License Manager",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, Scale(32), hwnd, (HMENU)200, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 200, WM_SETFONT, (WPARAM)hFontBig, TRUE);
    y += Scale(42);

    // Separator line
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        x, y, w, Scale(2), hwnd, NULL, hInstance, NULL);
    y += Scale(16);

    // Status label
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, Scale(28), hwnd, (HMENU)ID_STATUS_LABEL, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_STATUS_LABEL, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(38);

    // Update notification banner (only when update available)
    if (Info.updateAvailable) {
        // Background panel for update banner
        CreateWindowA("STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            x - Scale(8), y - Scale(8), w + Scale(16), Scale(90), hwnd, (HMENU)ID_UPDATE_PANEL, hInstance, NULL);
        
        // Update text (multi-line, word wrap)
        CreateWindowA("STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, w - Scale(220), Scale(60), hwnd, (HMENU)ID_UPDATE_LABEL, hInstance, NULL);
        SendDlgItemMessageA(hwnd, ID_UPDATE_LABEL, WM_SETFONT, (WPARAM)hFontMed, TRUE);

        // Update button (larger, more visible)
        CreateWindowA("BUTTON", "Update && Restart VDJ",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x + w - Scale(210), y + Scale(15), Scale(210), Scale(44), hwnd, (HMENU)ID_UPDATE_BTN, hInstance, NULL);
        SendDlgItemMessageA(hwnd, ID_UPDATE_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);
        y += Scale(95);
    }

    // === LICENSE STATUS SECTION (Marketplace Style) ===
    // License Status label
    CreateWindowA("STATIC", "License Status:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, Scale(140), Scale(22), hwnd, (HMENU)210, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 210, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + Scale(145), y, Scale(380), Scale(22), hwnd, (HMENU)211, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 211, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(30);

    // Expires label
    CreateWindowA("STATIC", "Expires:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, Scale(140), Scale(22), hwnd, (HMENU)212, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 212, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + Scale(145), y, Scale(380), Scale(22), hwnd, (HMENU)213, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 213, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(30);

    // License Type label
    CreateWindowA("STATIC", "License Type:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, Scale(140), Scale(22), hwnd, (HMENU)214, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 214, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + Scale(145), y, Scale(380), Scale(22), hwnd, (HMENU)215, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 215, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(30);

    // Activations label
    CreateWindowA("STATIC", "Activations:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, Scale(140), Scale(22), hwnd, (HMENU)216, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 216, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + Scale(145), y, Scale(380), Scale(22), hwnd, (HMENU)217, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 217, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(42);

    // Separator
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        x, y, w, 2, hwnd, NULL, hInstance, NULL);
    y += 14;

    // === ACCOUNT LOGIN SECTION ===
    CreateWindowA("STATIC", "Account Login",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, Scale(22), hwnd, (HMENU)230, hInstance, NULL);
    SendDlgItemMessageA(hwnd, 230, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(30);

    // Email label
    CreateWindowA("STATIC", "Email:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, Scale(60), Scale(22), hwnd, (HMENU)ID_ACCOUNT_LABEL, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_ACCOUNT_LABEL, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(28);

    // Email input
    CreateWindowA("EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, y, Scale(280), Scale(34), hwnd, (HMENU)ID_EMAIL_EDIT, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_EMAIL_EDIT, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    SendDlgItemMessageA(hwnd, ID_EMAIL_EDIT, EM_SETCUEBANNER, TRUE, (LPARAM)L"Email");

    // Password input
    CreateWindowA("EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
        x + Scale(290), y, Scale(180), Scale(34), hwnd, (HMENU)ID_PASSWORD_EDIT, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_PASSWORD_EDIT, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    SendDlgItemMessageA(hwnd, ID_PASSWORD_EDIT, EM_SETCUEBANNER, TRUE, (LPARAM)L"Password");

    // Login button
    CreateWindowA("BUTTON", "Login",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + Scale(480), y, Scale(70), Scale(34), hwnd, (HMENU)ID_LOGIN_BTN, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_LOGIN_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);

    // Logout button (initially hidden)
    CreateWindowA("BUTTON", "Logout",
        WS_CHILD | BS_PUSHBUTTON,
        x, y, Scale(80), Scale(34), hwnd, (HMENU)ID_LOGOUT_BTN, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_LOGOUT_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);

    // Dashboard button (initially hidden)
    CreateWindowA("BUTTON", "Open Dashboard",
        WS_CHILD | BS_PUSHBUTTON,
        x + Scale(90), y, Scale(150), Scale(34), hwnd, (HMENU)ID_DASHBOARD_BTN, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_DASHBOARD_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);
    y += Scale(52);

    // Separator
    CreateWindowA("STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        x, y, w, Scale(2), hwnd, NULL, hInstance, NULL);
    y += Scale(18);

    // Bottom buttons: Remove License + Close
    CreateWindowA("BUTTON", "Remove License",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, Scale(180), Scale(38), hwnd, (HMENU)ID_REMOVE_BTN, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_REMOVE_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);

    CreateWindowA("BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x + w - Scale(120), y, Scale(120), Scale(38), hwnd, (HMENU)ID_CLOSE_BTN, hInstance, NULL);
    SendDlgItemMessageA(hwnd, ID_CLOSE_BTN, WM_SETFONT, (WPARAM)hFontMed, TRUE);
}

void LicenseDialog::UpdateDisplay(HWND hwnd)
{
    // Status text — driven by serverStatus when available
    std::string statusText;
    if (Info.serverStatus == "active") {
        statusText = "[OK] License Active  (verified with server)";
    } else if (Info.serverStatus == "revoked") {
        statusText = "[X] License REVOKED by server - key has been disabled";
    } else if (Info.serverStatus == "expired") {
        statusText = "[!] License EXPIRED - please renew";
    } else if (Info.serverStatus == "no_key") {
        statusText = "[X] Unlicensed  (" + std::to_string(Info.freeSearches) + "/" + std::to_string(Info.maxSearches) + " free searches used)";
    } else if (Info.serverStatus == "no_server") {
        // Offline fallback
        if (Info.isLicensed) {
            statusText = "[OK] License Active  (offline)";
        } else if (!Info.licenseKey.empty()) {
            statusText = "[!] License Expired or Invalid";
        } else {
            statusText = "[X] Unlicensed  (" + std::to_string(Info.freeSearches) + "/" + std::to_string(Info.maxSearches) + " free searches used)";
        }
    } else {
        // Fallback for empty serverStatus
        if (Info.isLicensed) {
            statusText = "[OK] License Active";
        } else if (!Info.licenseKey.empty()) {
            statusText = "[!] License Expired or Invalid";
        } else {
            statusText = "[X] Unlicensed  (" + std::to_string(Info.freeSearches) + "/" + std::to_string(Info.maxSearches) + " free searches used)";
        }
    }
    SetDlgItemTextA(hwnd, ID_STATUS_LABEL, statusText.c_str());

    if (Info.updateAvailable) {
        std::string upd = "New version v" + Info.newVersion + " available\r\n" + Info.changelog;
        SetDlgItemTextA(hwnd, ID_UPDATE_LABEL, upd.c_str());
    }

    // License Status (Active/Inactive)
    if (Info.isLicensed) {
        SetDlgItemTextA(hwnd, 211, "Active");
    } else {
        SetDlgItemTextA(hwnd, 211, "Inactive");
    }

    // Expires
    if (Info.expiryDate.empty()) {
        SetDlgItemTextA(hwnd, 213, "N/A");
    } else {
        SetDlgItemTextA(hwnd, 213, Info.expiryDate.c_str());
    }

    // License Type
    if (Info.expiryDate.empty() && Info.isLicensed) {
        SetDlgItemTextA(hwnd, 215, "lifetime");
    } else if (!Info.expiryDate.empty()) {
        SetDlgItemTextA(hwnd, 215, "monthly");
    } else {
        SetDlgItemTextA(hwnd, 215, "-");
    }

    // Activations
    if (Info.deviceCount > 0 || Info.maxDevices > 0) {
        std::string activations = std::to_string(Info.deviceCount) + " / " + std::to_string(Info.maxDevices) + " devices";
        SetDlgItemTextA(hwnd, 217, activations.c_str());
    } else {
        SetDlgItemTextA(hwnd, 217, "-");
    }

    // Remove button only enabled if there's a key
    EnableWindow(GetDlgItem(hwnd, ID_REMOVE_BTN), !Info.licenseKey.empty());

    // Update marketplace account section
    if (Info.isLoggedIn) {
        // Show logged in state
        std::string accountText = "Logged in as: " + Info.accountName + " (" + Info.accountEmail + ")";
        if (Info.deviceCount > 0 || Info.maxDevices > 0) {
            accountText += "\nDevices: " + std::to_string(Info.deviceCount) + " / " + std::to_string(Info.maxDevices);
        }
        SetDlgItemTextA(hwnd, ID_ACCOUNT_LABEL, accountText.c_str());
        
        // Hide login controls, show logout/dashboard
        ShowWindow(GetDlgItem(hwnd, ID_EMAIL_EDIT), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_EDIT), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_LOGIN_BTN), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_LOGOUT_BTN), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_DASHBOARD_BTN), SW_SHOW);
    } else {
        // Show login form
        SetDlgItemTextA(hwnd, ID_ACCOUNT_LABEL, "Login to manage your licenses and devices:");
        
        // Show login controls, hide logout/dashboard
        ShowWindow(GetDlgItem(hwnd, ID_EMAIL_EDIT), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_EDIT), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_LOGIN_BTN), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_LOGOUT_BTN), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_DASHBOARD_BTN), SW_HIDE);
    }

    // Force repaint for colored labels
    InvalidateRect(hwnd, NULL, TRUE);
}

void LicenseDialog::HandleActivate(HWND hwnd)
{
    char buf[64] = {};
    GetDlgItemTextA(hwnd, ID_KEY_EDIT, buf, sizeof(buf));
    std::string key(buf);

    if (key.empty() || key == "YT-") {
        MessageBoxA(hwnd, "Please enter a valid license key.", "Activate", MB_OK | MB_ICONWARNING);
        return;
    }

    if (OnActivate) {
        // Show "checking..." in the status
        SetDlgItemTextA(hwnd, ID_STATUS_LABEL, "Checking license with server...");
        EnableWindow(GetDlgItem(hwnd, ID_ACTIVATE_BTN), FALSE);
        UpdateWindow(hwnd);

        bool success = OnActivate(key);

        EnableWindow(GetDlgItem(hwnd, ID_ACTIVATE_BTN), TRUE);

        if (success) {
            // Pull fresh info from plugin so display is up to date
            if (OnRefreshInfo) {
                Info = OnRefreshInfo();
            }
            MessageBoxA(hwnd,
                ("License activated successfully!\nValid until: " + Info.expiryDate).c_str(),
                "Activation Successful", MB_OK | MB_ICONINFORMATION);
            // Clear the input
            SetDlgItemTextA(hwnd, ID_KEY_EDIT, "YT-");
        } else {
            MessageBoxA(hwnd,
                "Activation failed.\n\nPossible reasons:\n"
                "- Key not found on server\n"
                "- Key already activated on another machine\n"
                "- Key has expired\n\n"
                "Contact support with your Machine ID shown above.",
                "Activation Failed", MB_OK | MB_ICONERROR);
        }
        UpdateDisplay(hwnd);
    }
}

void LicenseDialog::HandleRemove(HWND hwnd)
{
    int res = MessageBoxA(hwnd,
        "Are you sure you want to remove the license from this machine?\n\nYou can re-activate with the same key if needed.",
        "Remove License", MB_YESNO | MB_ICONQUESTION);

    if (res == IDYES && OnRemove) {
        OnRemove();
        UpdateDisplay(hwnd);
        MessageBoxA(hwnd, "License removed.", "Done", MB_OK | MB_ICONINFORMATION);
    }
}

void LicenseDialog::HandleCopyMachineID(HWND hwnd)
{
    if (Info.machineID.empty()) return;
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, Info.machineID.size() + 1);
        if (hMem) {
            memcpy(GlobalLock(hMem), Info.machineID.c_str(), Info.machineID.size() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
        MessageBoxA(hwnd, "Machine ID copied to clipboard.", "Copied", MB_OK | MB_ICONINFORMATION);
    }
}

void LicenseDialog::HandleUpdate(HWND hwnd)
{
    std::string msg = "Update to v" + Info.newVersion + "?";
    if (!Info.changelog.empty()) msg += "\n\n" + Info.changelog;
    msg += "\n\nVirtualDJ will automatically close and restart after installing.\nProceed?";

    if (MessageBoxA(hwnd, msg.c_str(), "Update Available", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        DestroyWindow(hwnd);
        if (OnUpdate) OnUpdate();
    }
}

void LicenseDialog::HandleLogin(HWND hwnd)
{
    char email[256] = {};
    char password[256] = {};
    GetDlgItemTextA(hwnd, ID_EMAIL_EDIT, email, sizeof(email));
    GetDlgItemTextA(hwnd, ID_PASSWORD_EDIT, password, sizeof(password));

    std::string emailStr(email);
    std::string passwordStr(password);

    if (emailStr.empty() || passwordStr.empty()) {
        MessageBoxA(hwnd, "Please enter both email and password.", "Login", MB_OK | MB_ICONWARNING);
        return;
    }

    if (OnLogin) {
        SetDlgItemTextA(hwnd, ID_ACCOUNT_LABEL, "Logging in...");
        EnableWindow(GetDlgItem(hwnd, ID_LOGIN_BTN), FALSE);
        UpdateWindow(hwnd);

        bool success = OnLogin(emailStr, passwordStr);

        EnableWindow(GetDlgItem(hwnd, ID_LOGIN_BTN), TRUE);

        if (success) {
            // Clear password field
            SetDlgItemTextA(hwnd, ID_PASSWORD_EDIT, "");
            
            // Refresh info
            if (OnRefreshInfo) {
                Info = OnRefreshInfo();
            }
            
            MessageBoxA(hwnd, "Login successful!", "Login", MB_OK | MB_ICONINFORMATION);
        } else {
            std::string errorMsg = "Login failed.\n\nPlease check:\n- Email: " + emailStr + "\n- Password is correct\n- Plugin identifier: soundcloud-source\n\nCheck the log file for details.";
            MessageBoxA(hwnd, errorMsg.c_str(), "Login Failed", MB_OK | MB_ICONERROR);
        }
        UpdateDisplay(hwnd);
    } else {
        MessageBoxA(hwnd, "OnLogin callback not set!", "Error", MB_OK | MB_ICONERROR);
    }
}

void LicenseDialog::HandleLogout(HWND hwnd)
{
    if (MessageBoxA(hwnd, "Are you sure you want to logout?", "Logout", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        if (OnLogout) {
            OnLogout();
            
            // Clear input fields
            SetDlgItemTextA(hwnd, ID_EMAIL_EDIT, "");
            SetDlgItemTextA(hwnd, ID_PASSWORD_EDIT, "");
            
            // Refresh info
            if (OnRefreshInfo) {
                Info = OnRefreshInfo();
            }
            
            UpdateDisplay(hwnd);
            MessageBoxA(hwnd, "Logged out successfully.", "Logout", MB_OK | MB_ICONINFORMATION);
        }
    }
}

void LicenseDialog::HandleOpenDashboard(HWND hwnd)
{
    if (OnOpenDashboard) {
        OnOpenDashboard();
    }
}

LRESULT CALLBACK LicenseDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LicenseDialog* dlg = g_pLicDlg;

    switch (msg) {
    case WM_CREATE:
        if (dlg) {
            dlg->hDlg = hwnd;
            dlg->CreateControls(hwnd);
            dlg->UpdateDisplay(hwnd);
        }
        return 0;

    case WM_ERASEBKGND:
        if (dlg) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wp, &rc, dlg->hBgBrush);
        }
        return 1;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND hCtrl = (HWND)lp;
        int id = GetDlgCtrlID(hCtrl);
        SetBkMode(hdc, TRANSPARENT);

        // Title
        if (id == 200) {
            SetTextColor(hdc, RGB(255, 60, 60));
            return (LRESULT)dlg->hBgBrush;
        }
        // Status label - color based on license state
        if (id == ID_STATUS_LABEL) {
            COLORREF c = dlg && dlg->Info.isLicensed ? CLR_GREEN :
                         dlg && !dlg->Info.licenseKey.empty() ? CLR_ORANGE : CLR_RED;
            SetTextColor(hdc, c);
            return (LRESULT)dlg->hBgBrush;
        }
        // Update panel - darker background
        if (id == ID_UPDATE_PANEL) {
            static HBRUSH hUpdatePanelBrush = CreateSolidBrush(RGB(40, 35, 25));
            return (LRESULT)hUpdatePanelBrush;
        }
        // Update label - bright yellow/white for visibility
        if (id == ID_UPDATE_LABEL) { 
            SetTextColor(hdc, RGB(255, 220, 100)); 
            static HBRUSH hUpdateBgBrush = CreateSolidBrush(RGB(40, 35, 25));
            return (LRESULT)hUpdateBgBrush; 
        }
        // Machine ID value - blue
        if (id == 211) { SetTextColor(hdc, CLR_BLUE);  return (LRESULT)dlg->hBgBrush; }
        // Key value - green if licensed, dim otherwise
        if (id == 213) {
            COLORREF c = dlg && dlg->Info.isLicensed ? CLR_GREEN : CLR_TEXT_DIM;
            SetTextColor(hdc, c);
            return (LRESULT)dlg->hBgBrush;
        }
        // Expiry - green if valid
        if (id == 215) {
            COLORREF c = dlg && dlg->Info.isLicensed ? CLR_GREEN : CLR_ORANGE;
            SetTextColor(hdc, c);
            return (LRESULT)dlg->hBgBrush;
        }
        // Server URL
        if (id == 217) {
            COLORREF c = dlg && !dlg->Info.serverURL.empty() ? CLR_BLUE : CLR_TEXT_DIM;
            SetTextColor(hdc, c);
            return (LRESULT)dlg->hBgBrush;
        }
        // Hint label
        if (id == 221) {
            SetTextColor(hdc, RGB(100, 100, 120));
            return (LRESULT)dlg->hBgBrush;
        }
        // All other static labels
        SetTextColor(hdc, CLR_TEXT_DIM);
        return (LRESULT)(dlg ? dlg->hBgBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, RGB(20, 22, 35));
        SetTextColor(hdc, RGB(200, 220, 255));
        static HBRUSH hEditBr = CreateSolidBrush(RGB(20, 22, 35));
        return (LRESULT)hEditBr;
    }

    case WM_CTLCOLORBTN:
        return (LRESULT)(dlg ? dlg->hBgBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));

    case WM_COMMAND:
        if (!dlg) break;
        switch (LOWORD(wp)) {
        case ID_UPDATE_BTN:   dlg->HandleUpdate(hwnd);   break;
        case ID_ACTIVATE_BTN: dlg->HandleActivate(hwnd); break;
        case ID_REMOVE_BTN:   dlg->HandleRemove(hwnd);   break;
        case ID_COPY_ID_BTN:  dlg->HandleCopyMachineID(hwnd); break;
        case ID_LOGIN_BTN:    dlg->HandleLogin(hwnd);    break;
        case ID_LOGOUT_BTN:   dlg->HandleLogout(hwnd);   break;
        case ID_DASHBOARD_BTN: dlg->HandleOpenDashboard(hwnd); break;
        case ID_CLOSE_BTN:
        case IDCANCEL:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}
