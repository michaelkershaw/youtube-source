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
#include "PluginVersion.h"
#include "YtDlpRunner.h"
#include "CacheIndex.h"
#ifdef _WIN32
#include "SearchDialog.h"
#include "WebViewHost.h"
#else
#include "WebViewHostMac.h"
#endif

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
};

#ifndef _WIN32
struct SearchResult {
    std::string title;
    std::string duration;
    std::string channel;
    std::string videoId;
    std::string url;
    bool cachedMp3 = false;
    bool cachedMp4 = false;
};
#endif

class CYouTubeSource : public IVdjPluginOnlineSource
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
    HRESULT VDJ_API GetFolderList(IVdjSubfoldersList* subfoldersList);
    HRESULT VDJ_API GetFolder(const char* folderUniqueId, IVdjTracksList* tracksList);
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

    // yt-dlp pipeline (Phase 1: piped processes, JSON output)
    YtDlpRunner Runner;
    CacheIndex Cache;
    std::vector<VideoInfo> SearchYouTube(const std::string& query, int limit, std::atomic<bool>* cancel = nullptr);
    std::string GetDirectStreamUrl(const std::string& videoId);
    std::string FetchVideoTitle(const std::string& videoId);
    void MaybeUpdateYtDlp();  // background self-update, max once/72h
    std::thread YtDlpUpdateThread;

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

    // WebView2 UI (v3 primary interface)
    WebViewHost* pWebHost = nullptr;
    std::thread WebHostThread;
    void OpenWebUI();
    void HandleWebMessage(const std::string& type, const std::string& payloadJson);
    void PushLicenseToWeb();
    void PushTracksToWeb(const char* msgType, const std::vector<VideoInfo>& tracks);
    void OpenSearchWindow();
    void LoadTrackFromSearch(const SearchResult& result, int deck);
    void StreamTrackFromSearch(const SearchResult& result, int deck);

    // Phase 3: playlists / trending / charts / history
    std::vector<VideoInfo> FetchPlaylist(const std::string& url, int limit = 200);
    std::vector<VideoInfo> FetchTrending();
    std::vector<VideoInfo> FetchCharts(const std::string& genre);
    void AddToHistory(const std::string& query);
    void LoadHistory();
    void SaveHistory();
    std::vector<std::pair<std::string, std::string>> SearchHistoryList;  // query, date
    std::mutex HistoryMutex;

    // Download history
    struct DownloadHistEntry { std::string videoId; std::string title; std::string format; std::string date; };
    void AddToDownloadHistory(const std::string& videoId, const std::string& title, const std::string& format);
    void LoadDownloadHistory();
    void SaveDownloadHistory();
    void PushDownloadHistoryToWeb();
    std::vector<DownloadHistEntry> DownloadHistoryList;
    std::mutex DownloadHistoryMutex;

    // Cached results for VDJ sidebar folders
    std::vector<VideoInfo> LastTrending;
    std::vector<VideoInfo> LastCharts;

#ifdef _WIN32
    // Search dialog (legacy Win32 fallback when WebView2 runtime is missing)
    SearchDialog* pSearchDialog = nullptr;
    std::thread SearchWindowThread;
    void ShowSearchWindow();
    
    // Plugin bitmap (logo)
    HBITMAP hPluginBitmap = nullptr;
    ULONG_PTR GdiplusToken = 0;
#endif
    
    // Settings
    std::string DownloadFormat = "mp3";  // "mp3" or "mp4"
    void LoadSettings();
    void SaveSettings();

    // License system
    enum LicenseStatus { 
        LIC_UNCHECKED, 
        LIC_VALID, 
        LIC_EXPIRED, 
        LIC_INVALID, 
        LIC_REVOKED, 
        LIC_ERROR 
    };
    LicenseStatus LicStatus = LIC_UNCHECKED;
    std::string LicenseKey = "";
    bool IsLicensed = false;
    std::string ExpiryDate = "";
    std::string TokenExpiry = "";           // 14-day token expiration
    std::string LicenseType = "";           // monthly/yearly/lifetime
    std::string LicenseMessage = "";        // Last server message
    std::string PluginDescription = "";
    std::string MachineID = "";  // Hardware fingerprint
    int PluginId = 0;                       // Plugin ID from marketplace
    int FreeSearchCount = 0;
    std::string LastKnownTime = "";  // Anti-clock-tamper: last verified time from server or system
    std::function<void(bool)> OnLicenseStatusChanged;  // Callback when license status changes
    std::thread LicenseValidationThread;  // Background validation thread
    std::thread LicenseTimerThread;  // Periodic validation timer thread
    std::thread LicenseDialogThread;  // License dialog thread
#ifdef _WIN32
    HWND hLicenseDialogWindow = NULL;  // Track license dialog window for updates
#endif
    std::atomic<bool> ValidationInProgress{false};
    std::atomic<bool> StopTimer{false};  // Flag to stop timer thread
    bool CheckLicense();
    void CheckLicenseBackground();  // Non-blocking background check
    void UpdateLicenseStatus(bool licensed);  // Update status and notify listeners
    void StartLicenseTimer();  // Start periodic license validation timer
    void StopLicenseTimer();  // Stop periodic license validation timer
    void SaveLicense();
    void LoadLicense();
    void ShowLicenseDialog();
    void LogActivation(const std::string& key, bool success);
    std::string GetComputerName();
    std::string GetMachineID();  // Hardware fingerprint
    
    // Marketplace account integration
    bool IsLoggedIn = false;
    std::string AccountEmail = "";
    std::string AccountName = "";
    std::string AuthToken = "";
    std::string SavedPassword;  // Encrypted password for convenience
    bool SavePasswordEnabled = false;  // User preference to save password
    int MaxActivations = 0;             // Device activation limit (renamed from MaxDevices)
    int CurrentActivations = 0;         // Current device count (renamed from DeviceCount)
    
    // Sub-license support
    bool IsSharedLicense = false;       // Is this a shared/sub-license?
    std::string SharedOwnerName = "";   // Name of license owner (if shared)
    std::string SharedOwnerEmail = "";  // Email of license owner (if shared)
    std::string SharedDate = "";        // Date license was shared
    int MaxSubLicenses = 0;             // Maximum sub-licenses allowed
    int CurrentSubLicenses = 0;         // Current active sub-licenses
    bool LoginToMarketplace(const std::string& email, const std::string& password);
    bool LoginToMarketplace(const std::string& email, const std::string& password, bool savePassword);
    void LogoutFromMarketplace();
    void OpenMarketplaceDashboard();
    std::string EncryptPassword(const std::string& plain);
    std::string DecryptPassword(const std::string& encrypted);

    // Online license validation
    std::string LicenseServerURL = "https://djeventsuite.cloud";  // License validation server (marketplace)
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
