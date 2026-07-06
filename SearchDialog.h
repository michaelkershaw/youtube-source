#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <windows.h>
#include <commctrl.h>
#include <ole2.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "gdiplus.lib")

struct SearchResult {
    std::string title;
    std::string duration;
    std::string channel;
    std::string videoId;
    std::string url;
    bool cachedMp3 = false;
    bool cachedMp4 = false;
};

// Download progress info
struct DownloadProgress {
    std::string trackTitle;
    float percent = 0.0f;       // 0-100
    std::string speed;           // e.g. "2.5 MiB/s"
    std::string eta;             // e.g. "00:12"
    std::string phase;           // "Downloading", "Converting", "Complete", "Error"
    bool active = false;
};

// Custom Windows message for license status updates
#define WM_LICENSE_STATUS_CHANGED (WM_USER + 200)

class SearchDialog {
public:
    using LoadCallback = std::function<void(const SearchResult& result, int deck)>;
    using StreamCallback = std::function<void(const SearchResult& result, int deck)>;
    using SearchCountCallback = std::function<void(int searchesUsed)>;
    using FormatChangeCallback = std::function<void(const std::string& format)>;
    using DownloadVideoCallback = std::function<void(const SearchResult& result)>;
    using LicenseRequiredCallback = std::function<void()>;

    SearchDialog();
    ~SearchDialog();

    void Show(HINSTANCE hInstance);
    void Hide();
    bool IsVisible() const;

    void SetToolsDirectory(const std::string& dir);
    void SetCacheDirectory(const std::string& dir);
    void SetLoadCallback(LoadCallback callback);
    void SetStreamCallback(StreamCallback callback);
    void SetSearchCountCallback(SearchCountCallback callback);
    void SetFormatChangeCallback(FormatChangeCallback callback);
    void SetDownloadVideoCallback(DownloadVideoCallback callback);
    void SetLicenseRequiredCallback(LicenseRequiredCallback callback);
    void SetFormat(const std::string& format);
    std::string GetFormat() const;
    void SetStatusText(const std::string& text);
    void SetDownloadProgress(const DownloadProgress& progress);
    void SetLicenseStatus(bool licensed, int searchesUsed, int searchesMax, const std::string& expiry, const std::string& machineID);
    void RefreshLicenseStatus();  // Refresh license display from current state
    HWND GetHWND() const { return hWnd; }

    // Logging methods
    void LogDebug(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);
    void LogError(const std::string& message);

private:
    void UpdateFonts();
    void ResizeControls(int width, int height);
    int Scale(int value) const;
    void UpdateScaling();
    int ScreenDPI = 96;

    // Window handles
    HWND hWnd = NULL;
    HWND hSearchBox = NULL;
    HWND hSearchBtn = NULL;
    HWND hResultsList = NULL;
    HWND hLoadDeckA = NULL;
    HWND hLoadDeckB = NULL;
    HWND hAddAutomix = NULL;
    HWND hAddSidelist = NULL;
    HWND hFormatBtn = NULL;
    HWND hStatusLabel = NULL;
    HWND hProgressBar = NULL;
    HWND hProgressLabel = NULL;
    HWND hHeader = NULL;
    HWND hTitleLabel = NULL;
    HWND hCloseBtn = NULL;
    WNDPROC OldHeaderWndProc = NULL;
    WNDPROC OldSearchBoxProc = NULL;

    // Fonts and Brushes
    HFONT hFont = NULL;
    HFONT hFontBold = NULL;
    
    // Logo
    HBITMAP hLogoBitmap = NULL;
    std::string LogoPath;
    HFONT hFontSmall = NULL;
    HFONT hFontHeader = NULL;
    HFONT hFontTitle = NULL;
    HFONT hFontIcon = NULL;
    HBRUSH hBgBrush = NULL;
    HBRUSH hHeaderBrush = NULL;
    HBRUSH hTopBarBrush = NULL;
    HBRUSH hEditBgBrush = NULL;
    HBRUSH hRowEvenBrush = NULL;
    HBRUSH hRowOddBrush = NULL;
    HBRUSH hHighlightBrush = NULL;
    HBRUSH hBtnBrush = NULL;
    HBRUSH hBtnHoverBrush = NULL;
    HBRUSH hAccentBrush = NULL;
    HBRUSH hProgressBgBrush = NULL;
    HBRUSH hProgressFillBrush = NULL;
    HBRUSH hDeckABrush = NULL;
    HBRUSH hDeckBBrush = NULL;
    HBRUSH hAutomixBrush = NULL;
    HBRUSH hSidelistBrush = NULL;
    HBRUSH hFormatMp3Brush = NULL;
    HBRUSH hFormatMp4Brush = NULL;
    HPEN hBtnBorderPen = NULL;
    HPEN hSeparatorPen = NULL;
    HPEN hEditBorderPen = NULL;
    HPEN hEditBorderFocusPen = NULL;
    HPEN hAccentPen = NULL;
    bool EditFocused = false;

    // Logo
    Gdiplus::Image* pLogo = nullptr;
    void LoadLogo();

    // Download progress state
    DownloadProgress CurrentProgress;

    // Drag and drop
    bool DragActive = false;
    int DragItemIndex = -1;
    POINT DragStartPt = {};

    // State
    std::string ToolsDir;
    std::string CacheDir;
    std::vector<SearchResult> Results;
    LoadCallback OnLoadTrack;
    StreamCallback OnStreamTrack;
    SearchCountCallback OnSearchCount;
    FormatChangeCallback OnFormatChange;
    DownloadVideoCallback OnDownloadVideo;
    LicenseRequiredCallback OnLicenseRequired;
    std::string CurrentFormat = "mp3";
    bool Searching = false;
    HINSTANCE hInst = NULL;
    int HoveredButton = -1;

    // Logging system
    std::string LogFilePath;
    std::mutex LogMutex;
    void InitializeLogging();
    void Log(const std::string& level, const std::string& message);
    std::string GetTimestamp();

    // License state
    bool LicenseValid = false;
    int SearchesUsed = 0;
    int SearchesMax = 5;
    std::string LicenseExpiry = "";
    std::string MachineID = "";

    // Control IDs
    static const int ID_SEARCH_BOX = 101;
    static const int ID_SEARCH_BTN = 102;
    static const int ID_RESULTS_LIST = 103;
    static const int ID_LOAD_DECK_A = 104;
    static const int ID_LOAD_DECK_B = 105;
    static const int ID_ADD_AUTOMIX = 112;
    static const int ID_ADD_SIDELIST = 113;
    static const int ID_FORMAT_BTN = 114;
    static const int ID_CTX_DOWNLOAD_VIDEO = 200;
    static const int ID_CTX_DOWNLOAD_MP3 = 201;
    static const int ID_CTX_STREAM_DECK_A = 202;
    static const int ID_CTX_STREAM_DECK_B = 203;
    static const int ID_CTX_STREAM_AUTOMIX = 204;
    static const int ID_CTX_STREAM_SIDELIST = 205;
    static const int ID_STATUS_LABEL = 106;
    static const int ID_PROGRESS_BAR = 108;
    static const int ID_PROGRESS_LABEL = 109;
    static const int ID_TITLE_LABEL = 110;
    static const int ID_CLOSE_BTN = 111;

    // Window class name
    static const wchar_t* WND_CLASS_NAME;

    // Static window procedures
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HeaderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SearchBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Instance methods
    void CreateControls();
    void OnSearch();
    void OnLoadToDeck(int deck);
    void PopulateResults();
    void DoSearch(const std::string& query);
    void DrawProgressBar(HDC hdc, RECT rc);
    void BeginDragDrop(int itemIndex);
    static DWORD WINAPI SearchThreadProc(LPVOID param);

    struct SearchThreadData {
        SearchDialog* dialog;
        std::string query;
    };
};

#endif
