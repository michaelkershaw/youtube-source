#pragma once
#include <string>
#include <functional>
#include <windows.h>

// Callback types for license actions
using ActivateKeyCallback = std::function<bool(const std::string& key)>;
using RemoveKeyCallback   = std::function<void()>;
using LoginCallback       = std::function<bool(const std::string& email, const std::string& password)>;
using LogoutCallback      = std::function<void()>;
using OpenDashboardCallback = std::function<void()>;

struct LicenseInfo {
    bool        isLicensed    = false;
    std::string licenseKey    = "";
    std::string machineID     = "";
    std::string expiryDate    = "";
    std::string serverURL     = "";
    std::string serverStatus  = "";  // "active","revoked","expired","no_server","checking"
    bool        updateAvailable = false;
    std::string newVersion      = "";
    std::string changelog       = "";
    int         freeSearches  = 0;
    int         maxSearches   = 5;
    // Marketplace account info
    bool        isLoggedIn    = false;
    std::string accountEmail  = "";
    std::string accountName   = "";
    int         deviceCount   = 0;
    int         maxDevices    = 0;
};

using RefreshInfoCallback = std::function<LicenseInfo()>;
using UpdateCallback      = std::function<void()>;

class LicenseDialog {
public:
    LicenseDialog();
    ~LicenseDialog();

    void Show(HINSTANCE hInst, HWND hParent = NULL);
    void SetInfo(const LicenseInfo& info);
    void SetActivateCallback(ActivateKeyCallback cb) { OnActivate = cb; }
    void SetRemoveCallback(RemoveKeyCallback cb)      { OnRemove = cb; }
    void SetRefreshCallback(RefreshInfoCallback cb)   { OnRefreshInfo = cb; }
    void SetUpdateCallback(UpdateCallback cb)         { OnUpdate = cb; }
    void SetLoginCallback(LoginCallback cb)           { OnLogin = cb; }
    void SetLogoutCallback(LogoutCallback cb)         { OnLogout = cb; }
    void SetOpenDashboardCallback(OpenDashboardCallback cb) { OnOpenDashboard = cb; }

private:
    static LRESULT CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void CreateControls(HWND hwnd);
    void UpdateDisplay(HWND hwnd);
    void PaintBackground(HWND hwnd);
    void HandleActivate(HWND hwnd);
    void HandleRemove(HWND hwnd);
    void HandleCopyMachineID(HWND hwnd);
    void HandleUpdate(HWND hwnd);
    void HandleLogin(HWND hwnd);
    void HandleLogout(HWND hwnd);
    void HandleOpenDashboard(HWND hwnd);

    HWND hDlg           = NULL;
    HINSTANCE hInstance = NULL;

    // Control IDs
    enum {
        ID_KEY_EDIT     = 101,
        ID_ACTIVATE_BTN = 102,
        ID_REMOVE_BTN   = 103,
        ID_CLOSE_BTN    = 104,
        ID_COPY_ID_BTN  = 105,
        ID_STATUS_LABEL = 106,
        ID_UPDATE_BTN   = 107,
        ID_UPDATE_LABEL = 108,
        ID_UPDATE_PANEL = 109,
        ID_EMAIL_EDIT   = 110,
        ID_PASSWORD_EDIT = 111,
        ID_LOGIN_BTN    = 112,
        ID_LOGOUT_BTN   = 113,
        ID_DASHBOARD_BTN = 114,
        ID_ACCOUNT_LABEL = 115,
    };

    LicenseInfo         Info;
    ActivateKeyCallback OnActivate;
    RemoveKeyCallback   OnRemove;
    RefreshInfoCallback OnRefreshInfo;
    UpdateCallback      OnUpdate;
    LoginCallback       OnLogin;
    LogoutCallback      OnLogout;
    OpenDashboardCallback OnOpenDashboard;

    HBRUSH hBgBrush     = NULL;
    HBRUSH hPanelBrush  = NULL;
    HFONT  hFontBig     = NULL;
    HFONT  hFontMed     = NULL;
    HFONT  hFontMono    = NULL;
};
