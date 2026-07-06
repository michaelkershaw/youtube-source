#ifndef LICENSEDIALOGV2_H
#define LICENSEDIALOGV2_H
#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <mutex>
#include <fstream>

// Forward declaration for GDI+
namespace Gdiplus {
    class Image;
}

// YouTube-themed license dialog with dark/red colors matching YouTube branding
class LicenseDialogV2
{
public:
    LicenseDialogV2();
    ~LicenseDialogV2();
    
    // Show the dialog
    int ShowDialog(HWND parent);
    
    // Get window handle
    HWND GetHWND() const { return hDialog; }
    
    // Set license information
    void SetLicenseInfo(const std::string& key, const std::string& expiry, bool isLicensed);
    void SetMachineID(const std::string& machineId);
    void SetMarketplaceAccount(bool loggedIn, const std::string& email, int currentActivations, int maxActivations);
    void SetTrialInfo(int searchesUsed, int maxSearches);
    void SetSavedEmail(const std::string& email);  // Set saved email for pre-filling login form
    void SetSavedPassword(const std::string& password);  // Set saved password for pre-filling login form
    void SetSavePasswordEnabled(bool enabled);  // Set save password preference
    void RefreshLicenseDisplay();  // Refresh the dialog with current license data
    
    // Enhanced license setters
    void SetLicenseType(const std::string& type);
    void SetTokenExpiry(const std::string& expiry);
    void SetActivations(int current, int max);
    void SetSubLicenses(int current, int max);
    void SetSharedLicenseInfo(bool isShared, const std::string& ownerName, 
                              const std::string& ownerEmail, const std::string& sharedDate);
    
    // Callbacks
    using LoginCallback = std::function<bool(const std::string& email, const std::string& password, bool savePassword)>;
    using LogoutCallback = std::function<void()>;
    using DashboardCallback = std::function<void()>;
    
    void SetLoginCallback(LoginCallback callback) { OnLogin = callback; }
    void SetLogoutCallback(LogoutCallback callback) { OnLogout = callback; }
    void SetDashboardCallback(DashboardCallback callback) { OnDashboard = callback; }
    
    // Logging methods
    void LogDebug(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);
    void LogError(const std::string& message);
    
private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnInitDialog(HWND hwnd);
    void OnPaint(HWND hwnd);
    void OnCommand(HWND hwnd, WPARAM wParam);
    void DrawGradientBackground(HDC hdc, RECT rect);
    void DrawLogo(HDC hdc, int x, int y);
    
    HWND hDialog = NULL;
    HFONT hTitleFont = NULL;
    HFONT hNormalFont = NULL;
    HFONT hBoldFont = NULL;
    HBRUSH hOrangeBrush = NULL;
    HBRUSH hDarkBrush = NULL;
    HBITMAP hLogoBitmap = NULL;
    Gdiplus::Image* pLogoImage = NULL;
    std::string LogoPath;
    ULONG_PTR GdiplusToken = 0;
    
    std::string LicenseKey;
    std::string ExpiryDate;
    std::string MachineID;
    bool IsLicensed = false;
    
    bool IsLoggedIn = false;
    std::string AccountEmail;
    std::string SavedEmail;  // Email saved for pre-filling login form
    std::string SavedPassword;  // Password saved for pre-filling login form
    bool SavePasswordEnabled = false;  // User preference to save password
    int MaxActivations = 0;
    int CurrentActivations = 0;
    
    // Enhanced license fields
    std::string LicenseType;
    std::string TokenExpiry;
    std::string LicenseMessage;
    int MaxSubLicenses = 0;
    int CurrentSubLicenses = 0;
    
    // Sub-license info
    bool IsSharedLicense = false;
    std::string SharedOwnerName;
    std::string SharedOwnerEmail;
    std::string SharedDate;
    
    int SearchesUsed = 0;
    int MaxSearches = 5;
    
    LoginCallback OnLogin;
    LogoutCallback OnLogout;
    DashboardCallback OnDashboard;
    
    // Logging system
    std::string LogFilePath;
    std::mutex LogMutex;
    void InitializeLogging();
    void Log(const std::string& level, const std::string& message);
    std::string GetTimestamp();
    
    // Control IDs
    enum {
        IDC_EMAIL_EDIT = 1001,
        IDC_PASSWORD_EDIT = 1002,
        IDC_LOGIN_BTN = 1003,
        IDC_LOGOUT_BTN = 1004,
        IDC_DASHBOARD_BTN = 1005,
        IDC_CLOSE_BTN = 1006,
        IDC_COPY_MACHINEID_BTN = 1007,
        IDC_SAVE_PASSWORD_CHECK = 1008
    };
    
    // Colors (SoundCloud orange and dark theme)
    static const COLORREF COLOR_ORANGE = RGB(255, 85, 0);      // #FF5500
    static const COLORREF COLOR_DARK = RGB(17, 17, 17);        // #111111
    static const COLORREF COLOR_DARKER = RGB(10, 10, 10);      // #0A0A0A
    static const COLORREF COLOR_GRAY = RGB(102, 102, 102);     // #666666
    static const COLORREF COLOR_TEXT = RGB(224, 224, 224);     // #E0E0E0
    static const COLORREF COLOR_WHITE = RGB(255, 255, 255);    // #FFFFFF
};

#endif
