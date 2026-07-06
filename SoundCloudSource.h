#ifndef SOUNDCLOUDSOURCE_H
#define SOUNDCLOUDSOURCE_H

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <fstream>
#include <sstream>
#include "vdjPlugin8.h"
#include "vdjOnlineSource8.h"
#include "SearchDialog.h"
#include "PluginVersion.h"

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
};

class CSoundCloudSource : public IVdjPluginOnlineSource
{
public:
    // IVdjPlugin8 overrides
    HRESULT VDJ_API OnLoad();
    HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *infos);
    ULONG VDJ_API Release();

    // IVdjPluginOnlineSource overrides
    HRESULT VDJ_API OnSearch(const char* search, IVdjTracksList* tracksList);
    HRESULT VDJ_API OnSearchCancel();
    HRESULT VDJ_API GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage);
    HRESULT VDJ_API GetFolderContextMenu(const char* folderUniqueId, IVdjContextMenu* contextMenu);
    HRESULT VDJ_API OnFolderContextMenu(const char* folderUniqueId, size_t menuIndex);
    HRESULT VDJ_API GetContextMenu(const char* uniqueId, IVdjContextMenu* contextMenu);
    HRESULT VDJ_API OnContextMenu(const char* uniqueId, size_t menuIndex);
    
    // License activation
    bool ActivateLicense(const std::string& key);

private:
    // Logging system
    std::string LogFilePath;
    std::mutex LogMutex;
    LogLevel CurrentLogLevel = LOG_DEBUG;

    void InitializeLogging();
    void Log(LogLevel level, const std::string& message);
    void LogDebug(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);
    void LogError(const std::string& message);
    std::string GetTimestamp();
    std::string GetLogLevelString(LogLevel level);
    std::string GetModuleDirectory();
    std::string SanitizeFilename(const std::string& filename);

    // Command execution
    bool RunCommand(const std::string& command, std::vector<std::string>& output);

    // Download and cache
    std::string DownloadAndCacheTrack(const std::string& videoId, const std::string& title);
    void CleanupOldCache();
    void UpdateDownloadProgress(const std::string& title, const std::string& phase, float percent, const std::string& speed, const std::string& eta);

    // Search cancellation
    std::atomic<bool> SearchCancelled{false};

    // File and path management
    std::string ToolsDir;
    std::string ytDlpPath;
    std::string ffmpegPath;
    std::string ffmpegDir;
    std::string GlobalCacheDir;

    // Search dialog (kept for advanced search window)
    SearchDialog* pSearchDialog = nullptr;
    std::thread SearchWindowThread;
    std::thread LicenseDialogThread;
    HWND hLicenseDialogWindow = NULL;  // Track license dialog window for updates
    void ShowSearchWindow();
    void LoadTrackFromSearch(const SearchResult& result, int deck);
    void StreamTrackFromSearch(const SearchResult& result, int deck);
    void OpenSearchWindow();
    
    // Plugin bitmap (logo)
    HBITMAP hPluginBitmap = nullptr;
    ULONG_PTR GdiplusToken = 0;
    
    // Settings
    std::string DownloadFormat = "mp3";  // "mp3" or "mp4"
    void LoadSettings();
    void SaveSettings();

    // License system
    std::string LicenseKey = "";
    bool IsLicensed = false;
    std::string ExpiryDate = "";
    std::string PluginDescription = "";
    std::string MachineID = "";  // Hardware fingerprint
    int FreeSearchCount = 0;
    std::string LastKnownTime = "";  // Anti-clock-tamper: last verified time from server or system
    std::function<void(bool)> OnLicenseStatusChanged;  // Callback when license status changes
    std::thread LicenseValidationThread;  // Background validation thread
    std::thread LicenseTimerThread;  // Periodic validation timer thread
    std::atomic<bool> ValidationInProgress{false};
    std::atomic<bool> StopTimer{false};  // Flag to stop timer thread
    bool CheckLicense();
    void CheckLicenseBackground();  // Non-blocking background check
    void UpdateLicenseStatus(bool licensed);  // Update status and notify listeners
    void StartLicenseTimer();  // Start periodic license validation timer
    void StopLicenseTimer();  // Stop periodic license validation timer
    void SaveLicense();
    void LoadLicense();
    void ShowActivationDialog();
    void ShowLicenseDialog();
    void LogActivation(const std::string& key, bool success);
    std::string GetComputerName();
    std::string GetMachineID();
    
    // Marketplace account
    bool IsLoggedIn = false;
    std::string AccountEmail;
    std::string AccountName;
    std::string AuthToken;
    std::string SavedPassword;  // Encrypted password for convenience
    bool SavePasswordEnabled = false;  // User preference to save password
    int DeviceCount = 0;
    int MaxDevices = 0;
    
    // Password encryption helpers
    std::string EncryptPassword(const std::string& password);
    std::string DecryptPassword(const std::string& encrypted);
    
    bool LoginToMarketplace(const std::string& email, const std::string& password, bool savePassword = true);
    void LogoutFromMarketplace();
    void OpenMarketplaceDashboard();

    // Online license validation
    std::string LicenseServerURL = "https://djeventsuite.cloud";  // License validation server
    std::string HttpPost(const std::string& url, const std::string& postData);
    bool ValidateLicenseOnline();
    bool ActivateLicenseOnline(const std::string& key);

    // Auto-update
    bool        UpdateAvailable     = false;
    std::string UpdateNewVersion;
    std::string UpdateChangelog;
    std::string UpdateDownloadUrl64;
    std::string UpdateDownloadUrl32;
    void CheckForUpdates();
    bool HttpDownloadFile(const std::string& url, const std::string& localPath);
    void PerformUpdate();
};

#endif
