#include "YouTubeSource.h"
#ifdef _WIN32
#include "LicenseDialog.h"
#include "LicenseDialogV2.h"
#endif
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <shellapi.h>
#include <gdiplus.h>
#include <tlhelp32.h>
#pragma comment(lib, "gdiplus.lib")

HINSTANCE g_hInstance = NULL;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
#else
// ---- POSIX (macOS) portability shims ----
#include <dlfcn.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <cstdio>
#define localtime_s(tmptr, timeptr) localtime_r(timeptr, tmptr)
#define sprintf_s(buf, ...) snprintf(buf, sizeof(buf), __VA_ARGS__)
#define Sleep(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#ifndef E_POINTER
#define E_POINTER ((HRESULT)0x80004003L)
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif
#endif

// Open a URL in the default browser (cross-platform)
static void OpenUrlInBrowser(const std::string& url)
{
#ifdef _WIN32
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    std::string cmd = "open \"" + url + "\"";
    (void)system(cmd.c_str());
#endif
}

//////////////////////////////////////////////////////////////////////////
// Plugin lifecycle

HRESULT VDJ_API CYouTubeSource::OnLoad()
{
    // Try to initialize logging, but don't fail if it doesn't work
    try {
        InitializeLogging();
        LogInfo("YouTube Online Source Plugin loading...");
    } catch (...) {
        // Logging failed - continue without it
    }
    
    // Find tools in the dedicated "youtube-source" subfolder next to the DLL
    // (fallback: tools directly next to the DLL, for older installs)
    std::string dllDir = GetModuleDirectory();
#ifdef _WIN32
    const char* ytDlpName = "yt-dlp.exe";
    const char* ffmpegName = "ffmpeg.exe";
#else
    const char* ytDlpName = "yt-dlp";
    const char* ffmpegName = "ffmpeg";
#endif
    ToolsDir = dllDir + "/youtube-source";
    if (!std::filesystem::exists(ToolsDir + "/" + ytDlpName) &&
        std::filesystem::exists(dllDir + "/" + ytDlpName)) {
        ToolsDir = dllDir;
    }
    ytDlpPath = ToolsDir + "/" + ytDlpName;
    ffmpegPath = ToolsDir + "/" + ffmpegName;
    ffmpegDir = ToolsDir;
    
    LogInfo("DLL directory: " + dllDir);
    LogInfo("Tools directory: " + ToolsDir);
    LogInfo("yt-dlp: " + std::string(std::filesystem::exists(ytDlpPath) ? "FOUND" : "NOT FOUND"));
    LogInfo("ffmpeg: " + std::string(std::filesystem::exists(ffmpegPath) ? "FOUND" : "NOT FOUND"));
    
    // Create global cache directory
    std::filesystem::path cachePath;
#ifdef _WIN32
    char path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path) == S_OK) {
        cachePath = std::filesystem::path(path) / "VirtualDJ" / "youtube-cloud";
    } else {
        cachePath = std::filesystem::temp_directory_path() / "YouTubeSourceCache";
    }
#else
    const char* home = getenv("HOME");
    if (home && *home) {
        cachePath = std::filesystem::path(home) / "Documents" / "VirtualDJ" / "youtube-cloud";
    } else {
        cachePath = std::filesystem::temp_directory_path() / "YouTubeSourceCache";
    }
#endif
    std::filesystem::create_directories(cachePath);
    GlobalCacheDir = cachePath.string();
    LogInfo("Cache directory: " + GlobalCacheDir);
    
    // Initialize yt-dlp runner and cache index
    Runner.SetExePath(ytDlpPath);
    Runner.SetLogger([this](const std::string& msg) { LogDebug(msg); });
    Cache.Load(GlobalCacheDir);
    Cache.Reconcile();
    
    CleanupOldCache();
    
    // Load user settings (format preference, etc.)
    LoadSettings();
    LoadHistory();
    LoadDownloadHistory();
    
    // Generate machine ID for hardware binding
    MachineID = GetMachineID();
    LogInfo("Machine ID: " + MachineID);
    
    // Check license
    IsLicensed = CheckLicense();
    if (!IsLicensed) {
        LogWarning("Plugin is not licensed - some features will be disabled");
    }
    
#ifdef _WIN32
    // Initialize GDI+ for PNG loading
    Gdiplus::GdiplusStartupInput gdiplusInput;
    Gdiplus::GdiplusStartup(&GdiplusToken, &gdiplusInput, nullptr);
#endif
    
    // Start background validation (non-blocking)
    CheckLicenseBackground();
    
    // Start periodic license validation timer (every 15 seconds)
    StartLicenseTimer();
    
    // Check for updates in background (non-blocking)
    std::thread([this]() { CheckForUpdates(); }).detach();

    // Self-update yt-dlp in background (max once per 72h)
    YtDlpUpdateThread = std::thread([this]() { MaybeUpdateYtDlp(); });

    LogInfo("Plugin loaded successfully");
    return S_OK;
}

HRESULT VDJ_API CYouTubeSource::OnGetPluginInfo(TVdjPluginInfo8 *infos)
{
    // Two states: Unlicensed and Pro
    std::string pluginName, description;
    if (IsLicensed) {
        pluginName = "YouTube Search - Pro v" + std::string(PLUGIN_VERSION);
        description = "Search and play YouTube audio/video  |  Licensed until " + ExpiryDate;
    } else {
        pluginName = "YouTube Search - Unlicensed v" + std::string(PLUGIN_VERSION);
        description = "Search and play YouTube audio/video  |  License required";
    }
    
    infos->PluginName = pluginName.c_str();
    infos->Author = "Virtual DJ Plugin";
    PluginDescription = description;
    infos->Description = PluginDescription.c_str();
    infos->Version = "2.0";
    infos->Flags = 0;
    
#ifdef _WIN32
    // Load YouTube logo as bitmap from DLL resource (ID 7 = folder icon)
    if (!hPluginBitmap) {
        hPluginBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(7));
        if (hPluginBitmap) {
            LogInfo("Loaded YouTube logo from DLL resource ID 7");
        } else {
            LogWarning("Failed to load YouTube logo from DLL resource ID 7");
        }
    }
    infos->Bitmap = hPluginBitmap;
#endif
    
    return S_OK;
}

ULONG VDJ_API CYouTubeSource::Release()
{
    LogInfo("Plugin releasing...");
    SearchCancelled = true;
    if (YtDlpUpdateThread.joinable()) {
        YtDlpUpdateThread.detach();
    }
    if (pWebHost) {
        pWebHost->Close();
        if (WebHostThread.joinable()) WebHostThread.join();
        delete pWebHost;
        pWebHost = nullptr;
    }
#ifdef _WIN32
    if (pSearchDialog) {
        delete pSearchDialog;
        pSearchDialog = nullptr;
    }
    if (SearchWindowThread.joinable()) {
        SearchWindowThread.detach();
    }
    
    // Clean up bitmap
    if (hPluginBitmap) {
        DeleteObject(hPluginBitmap);
        hPluginBitmap = nullptr;
        LogInfo("Released plugin bitmap");
    }
    
    // Shut down GDI+
    if (GdiplusToken) {
        Gdiplus::GdiplusShutdown(GdiplusToken);
        GdiplusToken = 0;
    }
#endif
    
    delete this;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Search

HRESULT VDJ_API CYouTubeSource::OnSearch(const char* search, IVdjTracksList* tracksList)
{
    if (!search || !tracksList) return E_POINTER;
    if (search[0] == '\0') return S_OK;
    
    std::string query(search);
    
    // Handle license activation via search box: type ACTIVATE:YT-XXXX-XXXX-XXXX
    if (query.rfind("ACTIVATE:", 0) == 0) {
        std::string key = query.substr(9);
        if (ActivateLicense(key)) {
            LogActivation(key, true);
            tracksList->add("LICENSE_OK", "License Activated!",
                            ("Valid until " + ExpiryDate).c_str(),
                            nullptr, nullptr, nullptr, "YouTube Source is now fully unlocked");
        } else {
            LogActivation(key, false);
            std::string errorMsg = "Contact support with your Machine ID: " + MachineID;
            tracksList->add("LICENSE_ERR", "Activation Failed - Invalid Key",
                            "Keys must be bound to your machine ID",
                            nullptr, nullptr, nullptr, errorMsg.c_str());
        }
        tracksList->finish();
        return S_OK;
    }
    
    // License check - require license for all searches
    if (!IsLicensed) {
        LogWarning("Search attempted without license - opening License Manager");
        tracksList->add("LICENSE", "License Required",
                        "Please activate your license to search YouTube",
                        nullptr, nullptr, nullptr, "Click License Manager to continue");
        tracksList->finish();
        
        // Open License Manager for login/activation
        ShowLicenseDialog();
        return S_OK;
    }
    
    if (query.empty()) return S_OK;
    
    LogInfo("OnSearch called: \"" + query + "\"");
    SearchCancelled = false;
    
    std::vector<VideoInfo> results = SearchYouTube(query, 25, &SearchCancelled);
    if (SearchCancelled) {
        LogInfo("Search cancelled");
        return S_OK;
    }
    
    bool isVideoFormat = (DownloadFormat == "mp4");
    int count = 0;
    for (const auto& v : results) {
        std::string coverUrl = "https://i.ytimg.com/vi/" + v.id + "/hqdefault.jpg";
        tracksList->add(
            v.id.c_str(),            // uniqueId
            v.title.c_str(),         // title
            v.channel.c_str(),       // artist
            0,                       // remix
            0,                       // genre
            0,                       // label
            v.durationStr.c_str(),   // comment (show duration here)
            coverUrl.c_str(),        // coverUrl (YouTube thumbnail)
            0,                       // streamUrl (provided on demand via GetStreamUrl)
            (float)v.durationSec,    // length
            0,                       // bpm
            0,                       // key
            0,                       // year
            isVideoFormat,           // isVideo
            false                    // isKaraoke
        );
        count++;
        LogDebug("  Result: [" + v.id + "] " + v.title + " - " + v.channel + " (" + v.durationStr + ")");
    }
    
    tracksList->finish();
    LogInfo("Search returned " + std::to_string(count) + " results");
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// yt-dlp pipeline helpers (Phase 1)

std::vector<VideoInfo> CYouTubeSource::SearchYouTube(const std::string& query, int limit, std::atomic<bool>* cancel)
{
    std::vector<VideoInfo> results;
    
    std::vector<std::string> args = {
        "--no-warnings",
        "--flat-playlist",
        "--print", "%(.{id,title,uploader,duration,duration_string,view_count,webpage_url})j",
        "ytsearch" + std::to_string(limit) + ":" + query
    };
    
    auto r = Runner.RunWithRetry(args, 1, 60000, cancel);
    if (!r.ok()) {
        LogWarning("SearchYouTube: yt-dlp failed (exit=" + std::to_string(r.exitCode) +
                   (r.timedOut ? ", timeout" : "") + (r.cancelled ? ", cancelled" : "") + ")");
    }
    
    for (const auto& line : r.lines) {
        VideoInfo v;
        if (YtDlpRunner::ParseVideoJsonLine(line, v)) {
            results.push_back(std::move(v));
        }
    }
    LogInfo("SearchYouTube: parsed " + std::to_string(results.size()) + " results for \"" + query + "\"");
    return results;
}

std::string CYouTubeSource::GetDirectStreamUrl(const std::string& videoId)
{
    std::string videoUrl = "https://www.youtube.com/watch?v=" + videoId;
    std::string formatArg = (DownloadFormat == "mp4")
        ? "best[ext=mp4]/best"
        : "bestaudio[ext=m4a]/bestaudio";
    
    std::vector<std::string> args = {
        "--no-warnings", "--get-url", "-f", formatArg, videoUrl
    };
    
    auto r = Runner.RunWithRetry(args, 1, 30000);
    for (const auto& line : r.lines) {
        if (line.rfind("http", 0) == 0) return line;
    }
    return "";
}

std::string CYouTubeSource::FetchVideoTitle(const std::string& videoId)
{
    std::string videoUrl = "https://www.youtube.com/watch?v=" + videoId;
    std::vector<std::string> args = {
        "--no-warnings", "--print", "%(title)s", videoUrl
    };
    auto r = Runner.Run(args, 20000);
    for (const auto& line : r.lines) {
        if (line.rfind("WARNING:", 0) == 0 || line.rfind("ERROR:", 0) == 0) continue;
        if (!line.empty()) return line;
    }
    return "YouTube_" + videoId;
}

void CYouTubeSource::MaybeUpdateYtDlp()
{
    try {
        std::string stampFile = (std::filesystem::path(GlobalCacheDir) / "ytdlp_update_stamp.txt").string();
        long long lastUpdate = 0;
        {
            std::ifstream f(stampFile);
            if (f.good()) f >> lastUpdate;
        }
        long long now = (long long)std::time(nullptr);
        const long long interval = 72LL * 3600LL;  // 72 hours
        if (now - lastUpdate < interval) {
            LogDebug("MaybeUpdateYtDlp: last update " + std::to_string((now - lastUpdate) / 3600) + "h ago, skipping");
            return;
        }
        
        LogInfo("MaybeUpdateYtDlp: running yt-dlp self-update...");
        auto r = Runner.Run({ "-U" }, 180000);
        for (const auto& line : r.lines) LogInfo("yt-dlp -U: " + line);
        
        {
            std::ofstream f(stampFile, std::ios::trunc);
            f << now;
        }
    } catch (...) {
        LogWarning("MaybeUpdateYtDlp: exception during update check");
    }
}

HRESULT VDJ_API CYouTubeSource::OnSearchCancel()
{
    LogInfo("Search cancel requested");
    SearchCancelled = true;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Stream URL

HRESULT VDJ_API CYouTubeSource::GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage)
{
    if (!uniqueId) return E_POINTER;
    
    std::string videoId(uniqueId);
    LogInfo("GetStreamUrl called for: " + videoId);
    
    // Skip special IDs (license messages etc)
    if (videoId.find("LICENSE") == 0) return E_FAIL;
    
    std::string videoUrl = "https://www.youtube.com/watch?v=" + videoId;
    
    // Check if we already have a cached file for this video (exact match by videoId)
    std::string ext = (DownloadFormat == "mp4") ? ".mp4" : ".mp3";
    std::string cachedPath = (std::filesystem::path(GlobalCacheDir) / (videoId + ext)).string();
    if (std::filesystem::exists(cachedPath)) {
        LogInfo("GetStreamUrl: cache hit: " + cachedPath);
        url = cachedPath.c_str();
        return S_OK;
    }
    
    // Strategy 1: Get direct stream URL from yt-dlp (fast, ~1-2 seconds)
    {
        std::string directUrl = GetDirectStreamUrl(videoId);
        if (!directUrl.empty()) {
            LogInfo("GetStreamUrl: returning direct stream URL (" + std::to_string(directUrl.length()) + " chars)");
            url = directUrl.c_str();
            return S_OK;
        }
        LogWarning("GetStreamUrl: direct URL extraction failed, falling back to download");
    }
    
    // Strategy 2: Download and cache the file (slower, but reliable)
    std::string title = FetchVideoTitle(videoId);
    std::string sanitizedTitle = SanitizeFilename(title);
    std::string cachedFile = DownloadAndCacheTrack(videoId, sanitizedTitle);
    
    if (cachedFile.empty()) {
        errorMessage = "Failed to download track";
        LogError("GetStreamUrl: download failed for " + videoId);
        return E_FAIL;
    }
    
    url = cachedFile.c_str();
    LogInfo("GetStreamUrl: returning cached file " + cachedFile);
    return S_OK;
}

std::string CYouTubeSource::DownloadAndCacheTrack(const std::string& videoId, const std::string& title)
{
    std::string ext = (DownloadFormat == "mp4") ? ".mp4" : ".mp3";
    // Use videoId as filename to avoid yt-dlp sanitization mismatches
    std::string cacheFile = (std::filesystem::path(GlobalCacheDir) / (videoId + ext)).string();
    
    // Check cache first — since we use videoId as filename, this is always an exact match
    if (std::filesystem::exists(cacheFile)) {
        LogInfo("Cache hit: " + cacheFile);
        UpdateDownloadProgress(title, "Cached", 100.0f, "", "");
        return cacheFile;
    }
    
    // Show download starting
    UpdateDownloadProgress(title, "Downloading", 5.0f, "", "");
    
    // Download using yt-dlp with --newline so progress lines are flushed live
    std::string videoUrl = "https://www.youtube.com/watch?v=" + videoId;
    std::vector<std::string> args = { "--no-warnings", "--newline" };
    if (DownloadFormat == "mp4") {
        // Try 1080p first, then 720p, then best available
        args.insert(args.end(), {
            "-f", "(bestvideo[height<=1080][ext=mp4]+bestaudio[ext=m4a])/(bestvideo[height<=720][ext=mp4]+bestaudio[ext=m4a])/best[ext=mp4]/best",
            "--merge-output-format", "mp4"
        });
    } else {
        args.insert(args.end(), { "--extract-audio", "--audio-format", "mp3" });
    }
    args.insert(args.end(), {
        "--ffmpeg-location", ffmpegDir,
        "--add-metadata",
        "-o", cacheFile,
        videoUrl
    });
    
    std::string fmt = DownloadFormat;
    auto onLine = [this, &title, &fmt](const std::string& line) {
        // yt-dlp progress: [download]  45.2% of 4.52MiB at 2.31MiB/s ETA 00:02
        float pct = 0;
        std::string speed, eta;
        
        size_t pctPos = line.find('%');
        if (pctPos != std::string::npos) {
            size_t numStart = pctPos;
            while (numStart > 0 && (isdigit((unsigned char)line[numStart-1]) || line[numStart-1] == '.')) numStart--;
            try { pct = std::stof(line.substr(numStart, pctPos - numStart)); } catch (...) {}
        }
        
        size_t atPos = line.find(" at ");
        if (atPos != std::string::npos) {
            size_t speedEnd = line.find(' ', atPos + 4);
            if (speedEnd == std::string::npos) speedEnd = line.size();
            speed = line.substr(atPos + 4, speedEnd - atPos - 4);
        }
        
        size_t etaPos = line.find("ETA ");
        if (etaPos != std::string::npos) {
            eta = line.substr(etaPos + 4);
            while (!eta.empty() && (eta.back() == ' ' || eta.back() == '\r')) eta.pop_back();
        }
        
        std::string phase = "Downloading";
        if (line.find("[ExtractAudio]") != std::string::npos ||
            line.find("[Merger]") != std::string::npos ||
            line.find("Deleting original") != std::string::npos) {
            phase = (fmt == "mp4") ? "Merging MP4" : "Converting to MP3";
            pct = 90.0f;
        } else if (line.find("[Metadata]") != std::string::npos) {
            phase = "Adding metadata";
            pct = 95.0f;
        } else if (pct > 0) {
            pct = pct * 0.85f;  // Scale download to 0-85% range
        }
        
        if (pct > 0 || !phase.empty()) {
            UpdateDownloadProgress(title, phase, pct, speed, eta);
        }
    };
    
    auto r = Runner.Run(args, 300000, nullptr, onLine);  // 5 min max
    if (!r.ok()) {
        LogWarning("DownloadAndCacheTrack: yt-dlp exit=" + std::to_string(r.exitCode) +
                   (r.timedOut ? " (timeout)" : ""));
    }
    
    if (std::filesystem::exists(cacheFile)) {
        LogInfo("Download complete: " + cacheFile);
        UpdateDownloadProgress(title, "Complete", 100.0f, "", "");
        
        // Register in cache index
        CacheIndex::Entry e;
        e.videoId = videoId;
        e.title = title;
        e.format = DownloadFormat;
        e.path = cacheFile;
        e.addedAt = (long long)std::time(nullptr);
        Cache.Add(e);
        
        AddToDownloadHistory(videoId, title, DownloadFormat);
        
        return cacheFile;
    }
    
    LogError("Download failed, cache file not found: " + cacheFile);
    UpdateDownloadProgress(title, "Error", 0.0f, "", "");
    return "";
}

void CYouTubeSource::UpdateDownloadProgress(const std::string& title, const std::string& phase, float percent, const std::string& speed, const std::string& eta)
{
    // Push to WebView2 UI
    if (pWebHost) {
        bool active = (phase != "Complete" && phase != "Error" && phase != "Cached");
        nlohmann::json m = { {"type", "downloadProgress"}, {"payload", {
            {"title", title},
            {"phase", phase},
            {"percent", percent},
            {"speed", speed},
            {"eta", eta},
            {"active", active},
            {"format", DownloadFormat}
        }} };
        pWebHost->PostJson(m.dump());
    }
    
#ifdef _WIN32
    if (!pSearchDialog) return;
    
    DownloadProgress prog;
    prog.trackTitle = title;
    prog.phase = phase;
    prog.percent = percent;
    prog.speed = speed;
    prog.eta = eta;
    prog.active = (phase != "Complete" && phase != "Error" && phase != "Cached");
    
    pSearchDialog->SetDownloadProgress(prog);
    
    LogDebug("Download progress: " + phase + " " + std::to_string((int)percent) + "% " + speed + " " + eta);
#endif
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Context Menus

HRESULT VDJ_API CYouTubeSource::GetFolderContextMenu(const char* folderUniqueId, IVdjContextMenu* contextMenu)
{
    if (!contextMenu) return E_POINTER;
    LogInfo("GetFolderContextMenu called for: " + std::string(folderUniqueId ? folderUniqueId : "(root)"));
    
    // Add menu items
    contextMenu->add("Open Advanced Search Window");
    contextMenu->add("Manage License");
    
    LogInfo("Added 2 menu items to context menu");
    return S_OK;
}

HRESULT VDJ_API CYouTubeSource::OnFolderContextMenu(const char* folderUniqueId, size_t menuIndex)
{
    LogInfo("OnFolderContextMenu: index=" + std::to_string(menuIndex));
    
    if (menuIndex == 0) {
        OpenSearchWindow();
    } else if (menuIndex == 1) {
        ShowLicenseDialog();
    }
    return S_OK;
}

HRESULT VDJ_API CYouTubeSource::GetContextMenu(const char* uniqueId, IVdjContextMenu* contextMenu)
{
    if (!contextMenu || !uniqueId) return E_POINTER;
    LogInfo("GetContextMenu called for track: " + std::string(uniqueId));
    
    contextMenu->add("Open in Browser");
    return S_OK;
}

HRESULT VDJ_API CYouTubeSource::OnContextMenu(const char* uniqueId, size_t menuIndex)
{
    if (!uniqueId) return E_POINTER;
    LogInfo("OnContextMenu: uniqueId=" + std::string(uniqueId) + ", index=" + std::to_string(menuIndex));
    
    if (menuIndex == 0) {
        // Open the YouTube track in the default browser
        std::string url = "https://www.youtube.com/watch?v=" + std::string(uniqueId);
        OpenUrlInBrowser(url);
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// WebView2 UI (v3 primary interface)

static int DeckFromTarget(const std::string& target)
{
    if (target == "deckB") return 1;
    if (target == "automix") return 2;
    if (target == "sidelist") return 3;
    return 0; // deckA
}

void CYouTubeSource::OpenWebUI()
{
    if (pWebHost && pWebHost->IsVisible()) {
        pWebHost->BringToFront();
        return;
    }
    
    if (!pWebHost) {
        pWebHost = new WebViewHost();
        pWebHost->SetLogger([this](const std::string& m) { LogInfo(m); });
        pWebHost->SetMessageHandler([this](const std::string& type, const std::string& payload) {
            HandleWebMessage(type, payload);
        });
#ifdef _WIN32
    } else if (pWebHost->GetHWND()) {
        pWebHost->BringToFront();
        return;
    }
#else
    } else if (pWebHost->IsVisible()) {
        pWebHost->BringToFront();
        return;
    }
#endif
    
    if (WebHostThread.joinable()) WebHostThread.join();
    
    WebHostThread = std::thread([this]() {
#ifdef _WIN32
        HMODULE hModule = GetModuleHandleA("YouTubeSource64.dll");
        if (!hModule) hModule = GetModuleHandleA("YouTubeSource32.dll");
        HINSTANCE hInst = (HINSTANCE)hModule;
#else
        HINSTANCE hInst = nullptr;
#endif
        std::string uiDir = GetModuleDirectory() + "/youtube-source/ui";
        if (!std::filesystem::exists(uiDir + "/index.html"))
            uiDir = GetModuleDirectory() + "/ui";  // fallback: old location
        std::string userDataDir = GlobalCacheDir + "/webview";
        std::filesystem::create_directories(userDataDir);
        
        pWebHost->Show(hInst, L"YouTube Source", uiDir, userDataDir);
    });
}

void CYouTubeSource::PushTracksToWeb(const char* msgType, const std::vector<VideoInfo>& tracks)
{
    if (!pWebHost) return;
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& v : tracks) {
        arr.push_back({
            {"id", v.id},
            {"title", v.title},
            {"channel", v.channel},
            {"duration", v.durationStr},
            {"views", v.views},
            {"url", v.url},
            {"cachedMp3", Cache.Has(v.id, "mp3")},
            {"cachedMp4", Cache.Has(v.id, "mp4")}
        });
    }
    nlohmann::json msg = { {"type", msgType}, {"payload", { {"results", arr} }} };
    pWebHost->PostJson(msg.dump());
}

void CYouTubeSource::PushLicenseToWeb()
{
    if (!pWebHost) return;
    
    // Days remaining if expiry is YYYY-MM-DD
    int daysRemaining = -1;
    if (ExpiryDate.size() >= 10) {
        std::tm tmExp = {};
        std::istringstream ss(ExpiryDate.substr(0, 10));
        ss >> std::get_time(&tmExp, "%Y-%m-%d");
        if (!ss.fail()) {
            std::time_t exp = std::mktime(&tmExp);
            std::time_t now = std::time(nullptr);
            if (exp > 0) daysRemaining = (int)((exp - now) / 86400);
        }
    }
    
    nlohmann::json payload = {
        {"licensed", IsLicensed},
        {"expiry", ExpiryDate},
        {"machineId", MachineID},
        {"key", LicenseKey},
        {"type", LicenseType},
        {"statusText", LicenseMessage},
        {"account", AccountEmail},
        {"accountName", AccountName},
        {"loggedIn", IsLoggedIn}
    };
    if (daysRemaining >= 0) payload["daysRemaining"] = daysRemaining;
    if (MaxActivations > 0)
        payload["activations"] = std::to_string(CurrentActivations) + " / " + std::to_string(MaxActivations);
    
    nlohmann::json msg = { {"type", "licenseStatus"}, {"payload", payload} };
    pWebHost->PostJson(msg.dump());
}

void CYouTubeSource::HandleWebMessage(const std::string& type, const std::string& payloadJson)
{
    LogDebug("WebMessage: " + type);
    nlohmann::json p;
    try { p = nlohmann::json::parse(payloadJson); } catch (...) { p = nlohmann::json::object(); }
    
    auto pushStatus = [this](const std::string& text) {
        if (!pWebHost) return;
        nlohmann::json m = { {"type", "status"}, {"payload", { {"text", text} }} };
        pWebHost->PostJson(m.dump());
    };
    auto pushToast = [this](const std::string& text, const std::string& kind) {
        if (!pWebHost) return;
        nlohmann::json m = { {"type", "toast"}, {"payload", { {"text", text}, {"kind", kind} }} };
        pWebHost->PostJson(m.dump());
    };
    
    if (type == "uiReady") {
        nlohmann::json init = { {"type", "init"}, {"payload", {
            {"version", PLUGIN_VERSION},
            {"format", DownloadFormat}
        }} };
        pWebHost->PostJson(init.dump());
        PushLicenseToWeb();
        // Push history
        {
            std::lock_guard<std::mutex> lk(HistoryMutex);
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& h : SearchHistoryList)
                arr.push_back({ {"query", h.first}, {"date", h.second} });
            nlohmann::json m = { {"type", "historyResults"}, {"payload", { {"results", arr} }} };
            pWebHost->PostJson(m.dump());
        }
        PushDownloadHistoryToWeb();
        return;
    }
    
    if (type == "search") {
        if (!IsLicensed) {
            pushToast("License required - activate in the License tab", "err");
            nlohmann::json m = { {"type", "error"}, {"payload", { {"text", "License required to search"} }} };
            pWebHost->PostJson(m.dump());
            return;
        }
        std::string query = p.value("query", "");
        int limit = p.value("limit", 25);
        if (query.empty()) return;
        AddToHistory(query);
        std::thread([this, query, limit]() {
            SearchCancelled = false;
            auto results = SearchYouTube(query, limit, &SearchCancelled);
            PushTracksToWeb("searchResults", results);
        }).detach();
        return;
    }
    
    if (type == "importPlaylist") {
        std::string url = p.value("url", "");
        if (url.empty()) return;
        std::thread([this, url, pushToast]() {
            auto results = FetchPlaylist(url);
            PushTracksToWeb("playlistResults", results);
            if (results.empty()) pushToast("Playlist import failed or empty", "err");
        }).detach();
        return;
    }
    
    if (type == "getTrending") {
        std::thread([this]() {
            auto results = FetchTrending();
            LastTrending = results;
            PushTracksToWeb("trendingResults", results);
        }).detach();
        return;
    }
    
    if (type == "getCharts") {
        std::string genre = p.value("genre", "all");
        std::thread([this, genre]() {
            auto results = FetchCharts(genre);
            LastCharts = results;
            PushTracksToWeb("chartResults", results);
        }).detach();
        return;
    }
    
    if (type == "getHistory") {
        std::lock_guard<std::mutex> lk(HistoryMutex);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& h : SearchHistoryList)
            arr.push_back({ {"query", h.first}, {"date", h.second} });
        nlohmann::json m = { {"type", "historyResults"}, {"payload", { {"results", arr} }} };
        pWebHost->PostJson(m.dump());
        PushDownloadHistoryToWeb();
        return;
    }
    
    if (type == "loadTrack") {
        SearchResult r;
        r.videoId = p.value("videoId", "");
        r.title = p.value("title", r.videoId);
        r.url = "https://www.youtube.com/watch?v=" + r.videoId;
        int deck = DeckFromTarget(p.value("target", "deckA"));
        bool stream = (p.value("mode", "download") == "stream");
        std::thread([this, r, deck, stream]() {
            if (stream) StreamTrackFromSearch(r, deck);
            else LoadTrackFromSearch(r, deck);
        }).detach();
        return;
    }
    
    if (type == "downloadTrack") {
        std::string videoId = p.value("videoId", "");
        std::string title = SanitizeFilename(p.value("title", videoId));
        std::string format = p.value("format", "mp3");
        if (videoId.empty()) return;
        std::thread([this, videoId, title, format]() {
            std::string saved = DownloadFormat;
            DownloadFormat = format;
            DownloadAndCacheTrack(videoId, title);
            DownloadFormat = saved;
        }).detach();
        return;
    }
    
    if (type == "playlistBatch") {
        std::string action = p.value("action", "");
        auto tracks = p.value("tracks", nlohmann::json::array());
        std::thread([this, action, tracks, pushStatus, pushToast]() {
            int n = 0;
            for (const auto& t : tracks) {
                std::string videoId = t.value("videoId", "");
                std::string title = t.value("title", videoId);
                if (videoId.empty()) continue;
                if (action == "automix") {
                    SearchResult r;
                    r.videoId = videoId;
                    r.title = title;
                    r.url = "https://www.youtube.com/watch?v=" + videoId;
                    LoadTrackFromSearch(r, 2);  // automix
                } else if (action == "download") {
                    DownloadAndCacheTrack(videoId, SanitizeFilename(title));
                }
                n++;
                pushStatus("Batch " + action + ": " + std::to_string(n) + " / " + std::to_string(tracks.size()));
            }
            pushToast("Batch complete: " + std::to_string(n) + " tracks", "ok");
        }).detach();
        return;
    }
    
    if (type == "setFormat") {
        std::string fmt = p.value("format", "mp3");
        if (fmt == "mp3" || fmt == "mp4") {
            DownloadFormat = fmt;
            SaveSettings();
            LogInfo("Download format changed to: " + fmt);
        }
        return;
    }
    
    if (type == "openUrl") {
        std::string url = p.value("url", "");
        if (url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0)
            OpenUrlInBrowser(url);
        return;
    }
    
    // ===== License messages =====
    if (type == "license.activate") {
        std::string key = p.value("key", "");
        std::thread([this, key, pushToast]() {
            bool ok = ActivateLicense(key);
            LogActivation(key, ok);
            PushLicenseToWeb();
            pushToast(ok ? "License activated!" : "Activation failed - check your key", ok ? "ok" : "err");
        }).detach();
        return;
    }
    
    if (type == "license.login") {
        std::string email = p.value("email", "");
        std::string password = p.value("password", "");
        bool save = p.value("savePassword", false);
        std::thread([this, email, password, save, pushToast]() {
            bool ok = LoginToMarketplace(email, password, save);
            PushLicenseToWeb();
            pushToast(ok ? "Logged in" : "Login failed", ok ? "ok" : "err");
        }).detach();
        return;
    }
    
    if (type == "license.logout") {
        LogoutFromMarketplace();
        PushLicenseToWeb();
        return;
    }
    
    if (type == "license.openDashboard") {
        OpenMarketplaceDashboard();
        return;
    }
    
    LogWarning("HandleWebMessage: unknown type: " + type);
}

//////////////////////////////////////////////////////////////////////////
// Phase 3: playlists / trending / charts / history

std::vector<VideoInfo> CYouTubeSource::FetchPlaylist(const std::string& url, int limit)
{
    std::vector<VideoInfo> results;
    std::vector<std::string> args = {
        "--no-warnings",
        "--flat-playlist",
        "--playlist-end", std::to_string(limit),
        "--print", "%(.{id,title,uploader,duration,duration_string,view_count,webpage_url})j",
        url
    };
    auto r = Runner.RunWithRetry(args, 1, 120000);
    for (const auto& line : r.lines) {
        VideoInfo v;
        if (YtDlpRunner::ParseVideoJsonLine(line, v)) results.push_back(std::move(v));
    }
    LogInfo("FetchPlaylist: " + std::to_string(results.size()) + " tracks from " + url);
    return results;
}

std::vector<VideoInfo> CYouTubeSource::FetchTrending()
{
    // YouTube Charts: Top 100 music videos (maintained by YouTube)
    auto results = FetchPlaylist("https://www.youtube.com/playlist?list=PL4fGSI1pDJn6O1LS0XSdF3RyO0Rq_LDeI", 50);
    if (results.empty()) {
        results = SearchYouTube("trending music this week", 25);
    }
    return results;
}

std::vector<VideoInfo> CYouTubeSource::FetchCharts(const std::string& genre)
{
    // Genre -> curated search query map (server-side override can come later)
    std::string query;
    if (genre.empty() || genre == "all") query = "top songs this week";
    else query = "top " + genre + " songs this week";
    return SearchYouTube(query, 25);
}

void CYouTubeSource::AddToHistory(const std::string& query)
{
    std::lock_guard<std::mutex> lk(HistoryMutex);
    // Deduplicate: remove existing occurrence
    for (auto it = SearchHistoryList.begin(); it != SearchHistoryList.end();) {
        if (it->first == query) it = SearchHistoryList.erase(it);
        else ++it;
    }
    // Timestamp
    std::time_t now = std::time(nullptr);
    std::tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmNow);
    SearchHistoryList.insert(SearchHistoryList.begin(), { query, buf });
    if (SearchHistoryList.size() > 15) SearchHistoryList.resize(15);
    SaveHistory();
}

void CYouTubeSource::LoadHistory()
{
    std::lock_guard<std::mutex> lk(HistoryMutex);
    SearchHistoryList.clear();
    try {
        std::ifstream f(GlobalCacheDir + "/search_history.json");
        if (!f.good()) return;
        nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
        if (!j.is_array()) return;
        for (const auto& item : j) {
            SearchHistoryList.push_back({ item.value("query", ""), item.value("date", "") });
        }
    } catch (...) {}
}

void CYouTubeSource::SaveHistory()
{
    // Caller must hold HistoryMutex
    try {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& h : SearchHistoryList)
            j.push_back({ {"query", h.first}, {"date", h.second} });
        std::ofstream f(GlobalCacheDir + "/search_history.json", std::ios::trunc);
        f << j.dump(1);
    } catch (...) {}
}

void CYouTubeSource::AddToDownloadHistory(const std::string& videoId, const std::string& title, const std::string& format)
{
    {
        std::lock_guard<std::mutex> lk(DownloadHistoryMutex);
        // Deduplicate: remove existing occurrence of same video+format
        for (auto it = DownloadHistoryList.begin(); it != DownloadHistoryList.end();) {
            if (it->videoId == videoId && it->format == format) it = DownloadHistoryList.erase(it);
            else ++it;
        }
        std::time_t now = std::time(nullptr);
        std::tm tmNow;
        localtime_s(&tmNow, &now);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmNow);
        DownloadHistoryList.insert(DownloadHistoryList.begin(), { videoId, title, format, buf });
        if (DownloadHistoryList.size() > 200) DownloadHistoryList.resize(200);
        SaveDownloadHistory();
    }
    PushDownloadHistoryToWeb();
}

void CYouTubeSource::LoadDownloadHistory()
{
    std::lock_guard<std::mutex> lk(DownloadHistoryMutex);
    DownloadHistoryList.clear();
    try {
        std::ifstream f(GlobalCacheDir + "/download_history.json");
        if (!f.good()) return;
        nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
        if (!j.is_array()) return;
        for (const auto& item : j) {
            DownloadHistoryList.push_back({
                item.value("videoId", ""),
                item.value("title", ""),
                item.value("format", ""),
                item.value("date", "")
            });
        }
    } catch (...) {}
}

void CYouTubeSource::SaveDownloadHistory()
{
    // Caller must hold DownloadHistoryMutex
    try {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& d : DownloadHistoryList)
            j.push_back({ {"videoId", d.videoId}, {"title", d.title}, {"format", d.format}, {"date", d.date} });
        std::ofstream f(GlobalCacheDir + "/download_history.json", std::ios::trunc);
        f << j.dump(1);
    } catch (...) {}
}

void CYouTubeSource::PushDownloadHistoryToWeb()
{
    if (!pWebHost) return;
    std::lock_guard<std::mutex> lk(DownloadHistoryMutex);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& d : DownloadHistoryList)
        arr.push_back({ {"videoId", d.videoId}, {"title", d.title}, {"format", d.format}, {"date", d.date} });
    nlohmann::json m = { {"type", "downloadHistoryResults"}, {"payload", { {"results", arr} }} };
    pWebHost->PostJson(m.dump());
}

//////////////////////////////////////////////////////////////////////////
// VDJ sidebar folders (Trending / Charts / Cached)

HRESULT VDJ_API CYouTubeSource::GetFolderList(IVdjSubfoldersList* subfoldersList)
{
    if (!subfoldersList) return E_POINTER;
    subfoldersList->add("trending", "Trending");
    subfoldersList->add("charts", "Charts Top 100");
    subfoldersList->add("cached", "Cached Tracks");
    return S_OK;
}

HRESULT VDJ_API CYouTubeSource::GetFolder(const char* folderUniqueId, IVdjTracksList* tracksList)
{
    if (!folderUniqueId || !tracksList) return E_POINTER;
    std::string folder(folderUniqueId);
    LogInfo("GetFolder: " + folder);
    
    bool isVideoFormat = (DownloadFormat == "mp4");
    
    if (folder == "cached") {
        for (const auto& e : Cache.All()) {
            std::string coverUrl = "https://i.ytimg.com/vi/" + e.videoId + "/hqdefault.jpg";
            tracksList->add(
                e.videoId.c_str(), e.title.c_str(), e.channel.c_str(),
                0, 0, 0, e.duration.c_str(), coverUrl.c_str(),
                e.path.c_str(),  // local file streams instantly
                0, 0, 0, 0, e.format == "mp4", false);
        }
        tracksList->finish();
        return S_OK;
    }
    
    std::vector<VideoInfo> results;
    if (folder == "trending") {
        if (LastTrending.empty()) LastTrending = FetchTrending();
        results = LastTrending;
    } else if (folder == "charts") {
        if (LastCharts.empty()) LastCharts = FetchCharts("all");
        results = LastCharts;
    }
    
    for (const auto& v : results) {
        std::string coverUrl = "https://i.ytimg.com/vi/" + v.id + "/hqdefault.jpg";
        tracksList->add(
            v.id.c_str(), v.title.c_str(), v.channel.c_str(),
            0, 0, 0, v.durationStr.c_str(), coverUrl.c_str(),
            0, (float)v.durationSec, 0, 0, 0, isVideoFormat, false);
    }
    tracksList->finish();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// Search Dialog (Advanced Search Window)

void CYouTubeSource::OpenSearchWindow()
{
    // v3: prefer the WebView2 interface; fall back to the legacy Win32 dialog
    // if the WebView2 runtime is not installed.
    if (WebViewHost::IsRuntimeAvailable()) {
        OpenWebUI();
        return;
    }
#ifndef _WIN32
    // macOS: WKWebView is always available; no legacy fallback dialog
    LogError("OpenSearchWindow: no fallback UI available on this platform");
#else
    LogWarning("WebView2 runtime not available - using legacy search dialog");
    
    LogInfo("Opening advanced search window...");
    
    if (!pSearchDialog) {
        try {
            pSearchDialog = new SearchDialog();
            pSearchDialog->SetToolsDirectory(ToolsDir);
            pSearchDialog->SetCacheDirectory(GlobalCacheDir);
            pSearchDialog->SetFormat(DownloadFormat);
            LogInfo("SearchDialog created successfully");
        } catch (const std::exception& e) {
            LogError("Failed to create SearchDialog: " + std::string(e.what()));
            MessageBoxA(NULL, "Failed to create search dialog. Please check the log file for details.", 
                "Error", MB_OK | MB_ICONERROR);
            return;
        } catch (...) {
            LogError("Unknown error creating SearchDialog");
            MessageBoxA(NULL, "Failed to create search dialog. Please check the log file for details.", 
                "Error", MB_OK | MB_ICONERROR);
            return;
        }
        pSearchDialog->SetLoadCallback([this](const SearchResult& result, int deck) {
            LoadTrackFromSearch(result, deck);
        });
        pSearchDialog->SetStreamCallback([this](const SearchResult& result, int deck) {
            StreamTrackFromSearch(result, deck);
        });
        pSearchDialog->SetFormatChangeCallback([this](const std::string& format) {
            DownloadFormat = format;
            SaveSettings();
            LogInfo("Download format changed to: " + format);
        });
        pSearchDialog->SetDownloadVideoCallback([this](const SearchResult& result) {
            // Force download as MP4 regardless of current format setting
            std::thread([this, result]() {
                std::string videoId;
                std::string url = result.url;
                size_t pos = url.find("watch?v=");
                if (pos != std::string::npos) {
                    videoId = url.substr(pos + 8);
                    size_t ampPos = videoId.find('&');
                    if (ampPos != std::string::npos) videoId = videoId.substr(0, ampPos);
                } else {
                    videoId = url;
                }
                std::string sanitizedTitle = SanitizeFilename(result.title);
                // Temporarily force MP4 format
                std::string savedFormat = DownloadFormat;
                DownloadFormat = "mp4";
                std::string cachedFile = DownloadAndCacheTrack(videoId, sanitizedTitle);
                DownloadFormat = savedFormat;
                if (!cachedFile.empty()) {
                    if (pSearchDialog) pSearchDialog->SetStatusText("Video saved: " + result.title);
                } else {
                    if (pSearchDialog) pSearchDialog->SetStatusText("Video download failed!");
                }
            }).detach();
        });
        
        // Set up license change callback to update UI when license status changes
        OnLicenseStatusChanged = [this](bool licensed) {
            if (pSearchDialog) {
                pSearchDialog->SetLicenseStatus(licensed, 0, 1, ExpiryDate, MachineID);
            }
        };
        
        // Open License Manager when search requires license
        pSearchDialog->SetLicenseRequiredCallback([this]() {
            ShowLicenseDialog();
        });
    }
    ShowSearchWindow();
    // Push current license state to the dialog
    if (pSearchDialog) {
        pSearchDialog->SetLicenseStatus(IsLicensed, 0, 1, ExpiryDate, MachineID);
    }
#endif
}

#ifdef _WIN32
void CYouTubeSource::ShowSearchWindow()
{
    LogInfo("Showing search window...");
    if (SearchWindowThread.joinable()) {
        // Window already running, try to bring to front
        if (pSearchDialog && pSearchDialog->GetHWND()) {
            SetForegroundWindow(pSearchDialog->GetHWND());
            ShowWindow(pSearchDialog->GetHWND(), SW_SHOW);
            return;
        }
        // Wait for previous thread to finish
        SearchWindowThread.join();
    }
    
    SearchWindowThread = std::thread([this]() {
        HINSTANCE hInst = NULL;
#ifdef _WIN32
        HMODULE hModule = GetModuleHandleA("YouTubeSource64.dll");
        if (!hModule) hModule = GetModuleHandleA("YouTubeSource32.dll");
        hInst = (HINSTANCE)hModule;
#endif
        if (pSearchDialog) {
            pSearchDialog->Show(hInst);
            // Set initial license status
            pSearchDialog->SetLicenseStatus(IsLicensed, 0, 1, ExpiryDate, MachineID);
        }
    });
}
#endif // _WIN32 (ShowSearchWindow)

void CYouTubeSource::StreamTrackFromSearch(const SearchResult& result, int deck)
{
    std::string target;
    switch (deck) {
        case 0: target = "Deck A"; break;
        case 1: target = "Deck B"; break;
        case 2: target = "Automix"; break;
        case 3: target = "Sidelist"; break;
        default: target = "Deck A"; break;
    }
    LogInfo("StreamTrackFromSearch: " + result.title + " -> " + target);
#ifdef _WIN32
    if (pSearchDialog) pSearchDialog->SetStatusText("Getting stream URL: " + result.title);
#endif

    // Extract video ID
    std::string videoId = result.videoId;
    if (videoId.empty()) {
        std::string url = result.url;
        size_t pos = url.find("watch?v=");
        if (pos != std::string::npos) {
            videoId = url.substr(pos + 8);
            size_t ampPos = videoId.find('&');
            if (ampPos != std::string::npos) videoId = videoId.substr(0, ampPos);
        } else {
            videoId = url;
        }
    }
    LogInfo("StreamTrackFromSearch: videoId=" + videoId);

    // Use yt-dlp --get-url to obtain a direct streamable URL (~1-2 sec).
    // For video streaming a single muxed URL is used (split video+audio returns
    // 2 URLs which VDJ can't handle for streaming).
    std::string directUrl = GetDirectStreamUrl(videoId);

    if (directUrl.empty()) {
        LogWarning("StreamTrackFromSearch: no direct URL, falling back to download");
#ifdef _WIN32
        if (pSearchDialog) pSearchDialog->SetStatusText("Streaming failed, downloading instead...");
#endif
        LoadTrackFromSearch(result, deck);
        return;
    }

    LogInfo("StreamTrackFromSearch: direct URL obtained (" + std::to_string(directUrl.length()) + " chars)");

    // Send the direct stream URL to VDJ
    std::string vdjCommand;
    switch (deck) {
        case 0: vdjCommand = "deck 1 load \"" + directUrl + "\""; break;
        case 1: vdjCommand = "deck 2 load \"" + directUrl + "\""; break;
        case 2: vdjCommand = "automix_add_next \"" + directUrl + "\""; break;
        case 3: vdjCommand = "sidelist_add \"" + directUrl + "\""; break;
        default: vdjCommand = "deck 1 load \"" + directUrl + "\""; break;
    }
    LogInfo("StreamTrackFromSearch sending command");

    HRESULT hr = cb->SendCommand(vdjCommand.c_str());
    LogInfo("SendCommand HRESULT: 0x" + [](HRESULT h) {
        char buf[16]; sprintf_s(buf, "%08X", (unsigned)h); return std::string(buf);
    }(hr) + (SUCCEEDED(hr) ? " (SUCCESS)" : " (FAILED)"));

    if (FAILED(hr)) {
        LogWarning("StreamTrackFromSearch: SendCommand failed, falling back to download");
#ifdef _WIN32
        if (pSearchDialog) pSearchDialog->SetStatusText("Stream failed, downloading instead...");
#endif
        LoadTrackFromSearch(result, deck);
        return;
    }

#ifdef _WIN32
    if (pSearchDialog) pSearchDialog->SetStatusText("Streaming: " + result.title);
#endif
}

void CYouTubeSource::LoadTrackFromSearch(const SearchResult& result, int deck)
{
    std::string target;
    switch (deck) {
        case 0: target = "Deck A"; break;
        case 1: target = "Deck B"; break;
        case 2: target = "Automix"; break;
        case 3: target = "Sidelist"; break;
        default: target = "Deck A"; break;
    }
    LogInfo("Loading track from search dialog: " + result.title + " to " + target);
    
#ifdef _WIN32
    if (pSearchDialog) pSearchDialog->SetStatusText("Downloading: " + result.title);
#endif
    
    // Extract video ID from URL
    std::string videoId;
    std::string url = result.url;
    size_t pos = url.find("watch?v=");
    if (pos != std::string::npos) {
        videoId = url.substr(pos + 8);
        size_t ampPos = videoId.find('&');
        if (ampPos != std::string::npos) videoId = videoId.substr(0, ampPos);
    } else {
        // Might be a short URL or just an ID
        videoId = url;
    }
    
    LogInfo("Video ID: " + videoId);
    LogInfo("Original URL: " + result.url);
    
    std::string sanitizedTitle = SanitizeFilename(result.title);
    LogInfo("Sanitized title: " + sanitizedTitle);
    LogInfo("Download format: " + DownloadFormat);
    
    std::string cachedFile = DownloadAndCacheTrack(videoId, sanitizedTitle);
    
    if (cachedFile.empty()) {
        LogError("LoadTrackFromSearch: DownloadAndCacheTrack returned empty path for videoId=" + videoId);
#ifdef _WIN32
        if (pSearchDialog) pSearchDialog->SetStatusText("Download failed!");
#endif
        return;
    }
    
    // Verify file exists and get size before trying to load
    bool fileExists = std::filesystem::exists(cachedFile);
    std::uintmax_t fileSize = 0;
    if (fileExists) {
        try { fileSize = std::filesystem::file_size(cachedFile); } catch (...) {}
    }
    LogInfo("Cached file: " + cachedFile);
    LogInfo("File exists: " + std::string(fileExists ? "YES" : "NO"));
    LogInfo("File size: " + std::to_string(fileSize) + " bytes");
    
    if (!fileExists || fileSize == 0) {
        LogError("LoadTrackFromSearch: file missing or empty after download!");
#ifdef _WIN32
        if (pSearchDialog) pSearchDialog->SetStatusText("Error: file not found after download");
#endif
        return;
    }
    
    // Load into VDJ target
    std::string vdjCommand;
    switch (deck) {
        case 0: // Deck A
            vdjCommand = "deck 1 load \"" + cachedFile + "\"";
            break;
        case 1: // Deck B
            vdjCommand = "deck 2 load \"" + cachedFile + "\"";
            break;
        case 2: // Automix
            vdjCommand = "automix_add_next \"" + cachedFile + "\"";
            break;
        case 3: // Sidelist
            vdjCommand = "sidelist_add \"" + cachedFile + "\"";
            break;
        default:
            vdjCommand = "deck 1 load \"" + cachedFile + "\"";
            break;
    }
    LogInfo("Sending VDJ command: [" + vdjCommand + "]");
    LogInfo("cb pointer valid: " + std::string(cb ? "YES" : "NO"));
    
    HRESULT hr = cb->SendCommand(vdjCommand.c_str());
    LogInfo("SendCommand HRESULT: 0x" + [](HRESULT h) {
        char buf[16]; sprintf_s(buf, "%08X", (unsigned)h); return std::string(buf);
    }(hr) + (SUCCEEDED(hr) ? " (SUCCESS)" : " (FAILED)"));
    
    if (FAILED(hr)) {
        LogError("SendCommand failed - trying fallback: load_file");
        std::string fallback = "load_file \"" + cachedFile + "\"";
        LogInfo("Fallback command: [" + fallback + "]");
        HRESULT hr2 = cb->SendCommand(fallback.c_str());
        LogInfo("Fallback HRESULT: 0x" + [](HRESULT h) {
            char buf[16]; sprintf_s(buf, "%08X", (unsigned)h); return std::string(buf);
        }(hr2) + (SUCCEEDED(hr2) ? " (SUCCESS)" : " (FAILED)"));
        
        if (FAILED(hr2)) {
            LogError("Both commands failed. File: [" + cachedFile + "] Deck: " + target);
#ifdef _WIN32
            if (pSearchDialog) pSearchDialog->SetStatusText("Load failed - see log for details");
#endif
            return;
        }
    }
    
#ifdef _WIN32
    if (pSearchDialog) pSearchDialog->SetStatusText("Loaded: " + result.title);
#endif
}

//////////////////////////////////////////////////////////////////////////
// Command execution

bool CYouTubeSource::RunCommand(const std::string& command, std::vector<std::string>& output)
{
    LogDebug("Running command: " + command);
    
#ifdef _WIN32
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char* cmdLineBuf = _strdup(command.c_str());

    if (CreateProcessA(NULL, cmdLineBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 120000); // 2 minute timeout
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        free(cmdLineBuf);
        return exitCode == 0;
    }
    free(cmdLineBuf);
    LogError("CreateProcess failed for: " + command);
    return false;
#else
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line = buffer;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        if (!line.empty()) output.push_back(line);
    }
    int status = pclose(pipe);
    return status == 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
// Cache management

void CYouTubeSource::CleanupOldCache()
{
    LogInfo("Cleaning up cache files older than 12 hours...");
    try {
        if (GlobalCacheDir.empty() || !std::filesystem::exists(GlobalCacheDir)) return;

        // Index-based cleanup (mp3 + mp4, tracked with exact add time)
        int removed = Cache.Cleanup(12LL * 3600LL);
        if (removed > 0) LogInfo("CacheIndex cleanup removed " + std::to_string(removed) + " file(s)");

        auto now = std::chrono::system_clock::now();
        int count = 0;

        // Legacy sweep for untracked leftovers
        for (const auto& entry : std::filesystem::directory_iterator(GlobalCacheDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
                auto ftime = std::filesystem::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                auto age = std::chrono::duration_cast<std::chrono::hours>(now - sctp).count();
                if (age >= 12) {
                    std::filesystem::remove(entry.path());
                    count++;
                }
            }
        }
        if (count > 0) LogInfo("Removed " + std::to_string(count) + " expired cache files.");
    } catch (const std::exception& e) {
        LogError("Error cleaning up cache: " + std::string(e.what()));
    }
}

//////////////////////////////////////////////////////////////////////////
// Logging system

void CYouTubeSource::InitializeLogging()
{
    try {
        std::string moduleDir = GetModuleDirectory();
        LogFilePath = moduleDir + "/YouTubeSource.log";
        
        std::ofstream logFile(LogFilePath, std::ios::trunc);
        if (logFile.is_open()) {
            logFile.close();
            LogInfo("=== YouTube Online Source Plugin Log Started ===");
            LogInfo("DLL Location: " + moduleDir);
            LogInfo("Log file: " + LogFilePath);
        } else {
            std::filesystem::path tempLogPath = std::filesystem::temp_directory_path() / "YouTubeSource.log";
            LogFilePath = tempLogPath.string();
            
            std::ofstream fallbackLog(LogFilePath, std::ios::trunc);
            if (fallbackLog.is_open()) {
                fallbackLog.close();
                LogInfo("=== YouTube Online Source Plugin Log Started (Fallback) ===");
                LogInfo("DLL Location: " + moduleDir);
                LogInfo("Log file: " + LogFilePath);
            }
        }
    } catch (...) {}
}

void CYouTubeSource::Log(LogLevel level, const std::string& message)
{
    if (level < CurrentLogLevel) return;
    std::lock_guard<std::mutex> lock(LogMutex);
    try {
        std::ofstream logFile(LogFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << GetTimestamp() << " [" << GetLogLevelString(level) << "] " << message << std::endl;
        }
    } catch (...) {}
}

void CYouTubeSource::LogDebug(const std::string& message) { Log(LOG_DEBUG, message); }
void CYouTubeSource::LogInfo(const std::string& message) { Log(LOG_INFO, message); }
void CYouTubeSource::LogWarning(const std::string& message) { Log(LOG_WARNING, message); }
void CYouTubeSource::LogError(const std::string& message) { Log(LOG_ERROR, message); }

std::string CYouTubeSource::GetTimestamp()
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

std::string CYouTubeSource::GetLogLevelString(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string CYouTubeSource::GetModuleDirectory()
{
#ifdef _WIN32
    char path[MAX_PATH];
    
    if (g_hInstance) {
        if (GetModuleFileNameA(g_hInstance, path, MAX_PATH) > 0) {
            std::string fullPath(path);
            size_t lastSlash = fullPath.find_last_of('\\');
            if (lastSlash != std::string::npos) {
                return fullPath.substr(0, lastSlash);
            }
        }
    }
    
    HMODULE hModule = GetModuleHandleA("YouTubeSource64.dll");
    if (!hModule) hModule = GetModuleHandleA("YouTubeSource32.dll");
    
    if (hModule) {
        if (GetModuleFileNameA(hModule, path, MAX_PATH) > 0) {
            std::string fullPath(path);
            size_t lastSlash = fullPath.find_last_of('\\');
            if (lastSlash != std::string::npos) {
                return fullPath.substr(0, lastSlash);
            }
        }
    }
    
    return std::filesystem::current_path().string();
#else
    // macOS: locate the bundle/dylib containing this code via dladdr
    Dl_info info;
    if (dladdr((void*)&OpenUrlInBrowser, &info) && info.dli_fname) {
        std::filesystem::path p(info.dli_fname);
        return p.parent_path().string();
    }
    return std::filesystem::current_path().string();
#endif
}

std::string CYouTubeSource::SanitizeFilename(const std::string& filename)
{
    std::string sanitized = filename;
    const std::string invalidChars = "\\/:*?\"<>|";
    for (char& c : sanitized) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }
    return sanitized;
}

//////////////////////////////////////////////////////////////////////////
// Settings System

void CYouTubeSource::LoadSettings()
{
    std::string settingsFile = GlobalCacheDir + "/settings.ini";
    std::ifstream file(settingsFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Trim
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "format") {
                if (val == "mp3" || val == "mp4") {
                    DownloadFormat = val;
                }
            }
            else if (key == "license_server") {
                LicenseServerURL = val;
            }
        }
        file.close();
        LogInfo("Settings loaded: format=" + DownloadFormat);
    } else {
        LogInfo("No settings file found, using defaults");
    }
}

void CYouTubeSource::SaveSettings()
{
    std::string settingsFile = GlobalCacheDir + "/settings.ini";
    std::ofstream file(settingsFile);
    if (file.is_open()) {
        file << "format=" << DownloadFormat << std::endl;
        if (!LicenseServerURL.empty()) {
            file << "license_server=" << LicenseServerURL << std::endl;
        }
        file.close();
        LogInfo("Settings saved: format=" + DownloadFormat);
    } else {
        LogError("Failed to save settings to: " + settingsFile);
    }
}

//////////////////////////////////////////////////////////////////////////
// License System

void CYouTubeSource::LoadLicense()
{
    std::string licenseFile = GlobalCacheDir + "/license.dat";
    std::ifstream file(licenseFile);
    if (file.is_open()) {
        std::string countStr, boundMachineID, loginStr, devCountStr, maxDevStr;
        std::getline(file, LicenseKey);
        std::getline(file, ExpiryDate);
        if (std::getline(file, countStr)) {
            try { FreeSearchCount = std::stoi(countStr); } catch (...) { FreeSearchCount = 0; }
        }
        if (std::getline(file, boundMachineID)) {
            // Check if license is bound to this machine
            if (!boundMachineID.empty() && boundMachineID != MachineID) {
                LogWarning("License is bound to a different machine (" + boundMachineID + " != " + MachineID + ")");
                LicenseKey = "";  // Invalidate the license
                ExpiryDate = "";
                FreeSearchCount = 0;
            }
        }
        // Read last known time (anti-clock-tamper)
        if (std::getline(file, LastKnownTime)) {
            LogInfo("Loaded last known time: " + LastKnownTime);
        }
        // Read marketplace account data
        if (std::getline(file, loginStr)) {
            IsLoggedIn = (loginStr == "1");
        }
        std::getline(file, AccountEmail);
        std::getline(file, AccountName);
        std::getline(file, AuthToken);
        if (std::getline(file, devCountStr)) {
            try { CurrentActivations = std::stoi(devCountStr); } catch (...) { CurrentActivations = 0; }
        }
        if (std::getline(file, maxDevStr)) {
            try { MaxActivations = std::stoi(maxDevStr); } catch (...) { MaxActivations = 0; }
        }
        // Load enhanced license fields
        std::getline(file, TokenExpiry);
        std::getline(file, LicenseType);
        std::getline(file, LicenseMessage);
        std::string pluginIdStr;
        if (std::getline(file, pluginIdStr)) {
            try { PluginId = std::stoi(pluginIdStr); } catch (...) { PluginId = 0; }
        }
        std::string maxSubLicStr, currentSubLicStr, isSharedStr;
        if (std::getline(file, maxSubLicStr)) {
            try { MaxSubLicenses = std::stoi(maxSubLicStr); } catch (...) { MaxSubLicenses = 0; }
        }
        if (std::getline(file, currentSubLicStr)) {
            try { CurrentSubLicenses = std::stoi(currentSubLicStr); } catch (...) { CurrentSubLicenses = 0; }
        }
        if (std::getline(file, isSharedStr)) {
            IsSharedLicense = (isSharedStr == "1");
        }
        std::getline(file, SharedOwnerName);
        std::getline(file, SharedOwnerEmail);
        std::getline(file, SharedDate);
        file.close();
        LogInfo("Loaded license: key=" + LicenseKey + " expiry=" + ExpiryDate + " searches=" + std::to_string(FreeSearchCount) + " logged_in=" + (IsLoggedIn ? "yes" : "no"));
    }
}

void CYouTubeSource::SaveLicense()
{
    std::string licenseFile = GlobalCacheDir + "/license.dat";
    std::ofstream file(licenseFile);
    if (file.is_open()) {
        file << LicenseKey << std::endl;
        file << ExpiryDate << std::endl;
        file << FreeSearchCount << std::endl;
        file << MachineID << std::endl;  // Bind to current machine
        file << LastKnownTime << std::endl;  // Anti-clock-tamper timestamp
        file << (IsLoggedIn ? "1" : "0") << std::endl;  // Marketplace login state
        file << AccountEmail << std::endl;
        file << AccountName << std::endl;
        file << AuthToken << std::endl;
        file << CurrentActivations << std::endl;
        file << MaxActivations << std::endl;
        // Enhanced license fields
        file << TokenExpiry << std::endl;
        file << LicenseType << std::endl;
        file << LicenseMessage << std::endl;
        file << PluginId << std::endl;
        file << MaxSubLicenses << std::endl;
        file << CurrentSubLicenses << std::endl;
        file << (IsSharedLicense ? "1" : "0") << std::endl;
        file << SharedOwnerName << std::endl;
        file << SharedOwnerEmail << std::endl;
        file << SharedDate << std::endl;
        file.close();
    }
}

bool CYouTubeSource::CheckLicense()
{
    LoadLicense();
    
    if (LicenseKey.empty()) {
        LogInfo("No license key found");
        LicStatus = LIC_UNCHECKED;
        return false;
    }
    
    // Check if token has expired (14-day session timeout)
    if (!TokenExpiry.empty()) {
        std::tm tokenTm = {};
        std::istringstream tokenSs(TokenExpiry);
        tokenSs >> std::get_time(&tokenTm, "%Y-%m-%d %H:%M:%S");
        if (!tokenSs.fail()) {
            std::time_t tokenTime = std::mktime(&tokenTm);
            std::time_t nowTime = std::time(nullptr);
            if (nowTime > tokenTime) {
                LogWarning("Token expired - session timeout. Please login again.");
                LicStatus = LIC_EXPIRED;
                IsLicensed = false;
                return false;
            }
        }
    }
    
    // Try online validation first (if server URL is configured)
    if (!LicenseServerURL.empty()) {
        LogInfo("Attempting online license validation...");
        bool onlineResult = ValidateLicenseOnline();
        if (onlineResult) {
            LogInfo("Online validation: License ACTIVE until " + ExpiryDate);
            LicStatus = LIC_VALID;
            return true;
        }
        // If online says invalid/revoked/expired, trust the server
        // But if it was a connection failure (empty response), fall through to local check
    }
    
    // Fallback: local validation (cached license data from previous online validation)
    LogInfo("Using local license validation (cached data)...");
    
    // Validate key format - accept multiple formats:
    // 1. Marketplace format: YT-XXXX-XXXX-XXXX-XXXX-XXXX
    // 2. Server format: XXXX-XXXX-XXXX-XXXX (alphanumeric)
    std::regex marketplacePattern(R"(YT-\d{4}-\d{4}-\d{4}-\d{4}-\d{4})");
    std::regex serverPattern(R"([A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4})");
    
    if (!std::regex_match(LicenseKey, marketplacePattern) && !std::regex_match(LicenseKey, serverPattern)) {
        LogWarning("Invalid license key format: " + LicenseKey);
        return false;
    }
    
    LogInfo("License key format validated: " + LicenseKey);
    
    // Check if this is a lifetime license (no expiry date)
    std::string lowerType = LicenseType;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    bool isLifetime = (lowerType == "lifetime");
    
    // Check expiry date (not required for lifetime licenses)
    if (ExpiryDate.empty()) {
        if (isLifetime) {
            LogInfo("Lifetime license - no expiry date required");
            LicStatus = LIC_VALID;
            return true;
        } else {
            LogWarning("License has no expiry date and is not a lifetime license");
            return false;
        }
    }
    
    std::tm tm = {};
    std::istringstream ss(ExpiryDate);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        LogWarning("Invalid expiry date format: " + ExpiryDate);
        return false;
    }
    
    std::time_t expiry = std::mktime(&tm);
    std::time_t now = std::time(nullptr);
    
    // Anti-clock-tamper: detect if system clock was rolled back
    if (!LastKnownTime.empty()) {
        std::tm lktm = {};
        std::istringstream lkss(LastKnownTime);
        lkss >> std::get_time(&lktm, "%Y-%m-%d %H:%M:%S");
        if (!lkss.fail()) {
            std::time_t lastKnown = std::mktime(&lktm);
            // If system time is more than 24 hours behind last known server time, clock was rolled back
            if (now < lastKnown - 86400) {
                LogWarning("Clock tamper detected! System time is behind last known server time by " +
                    std::to_string((lastKnown - now) / 3600) + " hours. License invalidated.");
                return false;
            }
        }
    }
    
    if (now > expiry) {
        LogWarning("License expired on: " + ExpiryDate);
        return false;
    }
    
    // Update last known time with current system time (only moves forward)
    {
        std::time_t nowTime = std::time(nullptr);
        std::tm nowTm = {};
        localtime_s(&nowTm, &nowTime);
        std::ostringstream oss;
        oss << std::put_time(&nowTm, "%Y-%m-%d %H:%M:%S");
        std::string currentTime = oss.str();
        if (LastKnownTime.empty() || currentTime > LastKnownTime) {
            LastKnownTime = currentTime;
            SaveLicense();
        }
    }
    
    LogInfo("License valid until: " + ExpiryDate);
    return true;
}

void CYouTubeSource::ShowLicenseDialog()
{
#ifndef _WIN32
    // macOS: the License view lives inside the WebView UI
    OpenWebUI();
    return;
#else
    // If thread is still running, wait for it to finish
    if (LicenseDialogThread.joinable()) {
        LicenseDialogThread.join();
    }

    LicenseDialogThread = std::thread([this]() {
        // Refresh license status from server before opening dialog
        LogInfo("License Manager opened - refreshing license status from server...");
        CheckLicenseBackground();
        
        // Wait a moment for background validation to complete
        Sleep(500);
        
        // Load the latest license status
        LoadLicense();
        // Properly check license status - only a valid license key makes it Pro
        // Marketplace login (AuthToken) alone doesn't make it Pro
        IsLicensed = CheckLicense();

        LicenseDialogV2 dlg;
        dlg.SetLicenseInfo(LicenseKey, ExpiryDate, IsLicensed);
        dlg.SetMachineID(MachineID);
        dlg.SetMarketplaceAccount(IsLoggedIn, AccountEmail, CurrentActivations, MaxActivations);
        dlg.SetTrialInfo(0, 0);  // No trial - license required immediately
        dlg.SetSavedEmail(AccountEmail);  // Pre-fill email field with saved email
        dlg.SetSavedPassword(DecryptPassword(SavedPassword));  // Pre-fill password field with decrypted password
        dlg.SetSavePasswordEnabled(SavePasswordEnabled);  // Set checkbox state
        
        // Set enhanced license fields
        dlg.SetLicenseType(LicenseType);
        dlg.SetTokenExpiry(TokenExpiry);
        dlg.SetActivations(CurrentActivations, MaxActivations);
        dlg.SetSubLicenses(CurrentSubLicenses, MaxSubLicenses);
        dlg.SetSharedLicenseInfo(IsSharedLicense, SharedOwnerName, SharedOwnerEmail, SharedDate);

        dlg.SetLoginCallback([this](const std::string& email, const std::string& password, bool savePassword) -> bool {
            return LoginToMarketplace(email, password, savePassword);
        });

        dlg.SetLogoutCallback([this]() {
            LogoutFromMarketplace();
        });

        dlg.SetDashboardCallback([this]() {
            OpenMarketplaceDashboard();
        });

        dlg.ShowDialog(NULL);
        
        // Store window handle for updates
        hLicenseDialogWindow = dlg.GetHWND();
        
        // Clear window handle when dialog closes
        hLicenseDialogWindow = NULL;
        LogInfo("License dialog closed");
    });
    // Don't detach - let the thread complete naturally
#endif
}

bool CYouTubeSource::ActivateLicense(const std::string& key)
{
    // Online activation only (marketplace format: YT-XXXX-XXXX-XXXX-XXXX-XXXX)
    if (LicenseServerURL.empty()) {
        LogError("No license server URL configured - activation requires marketplace connection");
        return false;
    }
    
    std::regex keyPattern(R"(YT-\d{4}-\d{4}-\d{4}-\d{4}-\d{4})");
    if (!std::regex_match(key, keyPattern)) {
        LogWarning("Invalid license key format: " + key + " (expected: YT-XXXX-XXXX-XXXX-XXXX-XXXX)");
        return false;
    }
    
    LogInfo("Attempting online activation for key: " + key);
    if (ActivateLicenseOnline(key)) {
        return true;
    }
    
    LogError("Online activation failed for key: " + key);
    return false;
}

void CYouTubeSource::LogActivation(const std::string& key, bool success)
{
    // Get current timestamp
    std::time_t now = std::time(nullptr);
    std::tm tm = {};
    localtime_s(&tm, &now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    // Log to activations file in cache directory
    std::string logFile = GlobalCacheDir + "/activations.log";
    std::ofstream file(logFile, std::ios::app);
    if (file.is_open()) {
        file << "[" << timestamp << "] "
              << (success ? "SUCCESS" : "FAILED") 
              << " | KEY: " << key
              << " | MACHINE: " << GetComputerName()
              << std::endl;
        file.close();
    }
}

// Helper to get computer name
std::string CYouTubeSource::GetComputerName()
{
#ifdef _WIN32
    char buf[256] = {};
    DWORD size = sizeof(buf);
    if (GetComputerNameA(buf, &size)) {
        return std::string(buf);
    }
#else
    char buf[256] = {};
    if (gethostname(buf, sizeof(buf) - 1) == 0 && buf[0]) {
        return std::string(buf);
    }
#endif
    return "UNKNOWN";
}

// Generate hardware fingerprint
std::string CYouTubeSource::GetMachineID()
{
    // Get multiple hardware identifiers for a unique fingerprint
    std::string fingerprint;
    
#ifdef _WIN32
    // 1. Computer name
    char computerName[256] = {};
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        fingerprint += std::string(computerName);
    }
    
    // 2. Volume serial number of system drive
    char systemDir[MAX_PATH] = {};
    GetWindowsDirectoryA(systemDir, sizeof(systemDir));
    systemDir[3] = '\0'; // Extract drive letter (e.g., "C:\\")
    
    DWORD volumeSerialNumber;
    if (GetVolumeInformationA(systemDir, NULL, 0, &volumeSerialNumber, NULL, NULL, NULL, 0)) {
        char serial[16] = {};
        snprintf(serial, sizeof(serial), "%08X", volumeSerialNumber);
        fingerprint += serial;
    }
    
    // 3. Username
    char userName[256] = {};
    DWORD userNameSize = sizeof(userName);
    if (GetUserNameA(userName, &userNameSize)) {
        fingerprint += std::string(userName);
    }
#else
    // 1. Hostname
    char hostName[256] = {};
    if (gethostname(hostName, sizeof(hostName) - 1) == 0) {
        fingerprint += std::string(hostName);
    }
    
    // 2. Hardware UUID (IOPlatformUUID) - stable per Mac
    {
        FILE* p = popen("ioreg -rd1 -c IOPlatformExpertDevice | awk -F'\"' '/IOPlatformUUID/{print $4}'", "r");
        if (p) {
            char buf[128] = {};
            if (fgets(buf, sizeof(buf), p)) {
                std::string uuid(buf);
                while (!uuid.empty() && (uuid.back() == '\n' || uuid.back() == '\r')) uuid.pop_back();
                fingerprint += uuid;
            }
            pclose(p);
        }
    }
    
    // 3. Username
    if (const char* user = getenv("USER")) {
        fingerprint += user;
    }
#endif
    
    // Create hash of the fingerprint
    std::hash<std::string> hasher;
    size_t hash = hasher(fingerprint);
    
    // Convert to hex string (last 8 characters for brevity)
    char hashStr[16] = {};
    snprintf(hashStr, sizeof(hashStr), "%08X", (unsigned int)hash);
    return std::string(hashStr);
}

//////////////////////////////////////////////////////////////////////////
// Online License System (WinHTTP)

#ifdef _WIN32
std::string CYouTubeSource::HttpPost(const std::string& url, const std::string& postData)
{
    std::string result;
    
    // Parse URL to extract scheme, host, path
    std::string host, path;
    bool isHttps = false;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;
    
    std::string urlCopy = url;
    if (urlCopy.substr(0, 8) == "https://") {
        isHttps = true;
        port = INTERNET_DEFAULT_HTTPS_PORT;
        urlCopy = urlCopy.substr(8);
    } else if (urlCopy.substr(0, 7) == "http://") {
        urlCopy = urlCopy.substr(7);
    }
    
    size_t slashPos = urlCopy.find('/');
    if (slashPos != std::string::npos) {
        host = urlCopy.substr(0, slashPos);
        path = urlCopy.substr(slashPos);
    } else {
        host = urlCopy;
        path = "/";
    }
    
    // Check for port in host
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        try { port = (INTERNET_PORT)std::stoi(host.substr(colonPos + 1)); } catch (...) {}
        host = host.substr(0, colonPos);
    }
    
    HINTERNET hInternet = InternetOpenA("YouTubeSource/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) { LogError("HttpPost: InternetOpen failed"); return ""; }
    
    // Set timeouts
    DWORD timeout = 10000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION;
    if (isHttps) {
        flags |= INTERNET_FLAG_SECURE
               | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
               | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    }
    
    HINTERNET hConnect = InternetConnectA(hInternet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { LogError("HttpPost: InternetConnect failed, error=" + std::to_string(GetLastError())); InternetCloseHandle(hInternet); return ""; }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest) { LogError("HttpPost: HttpOpenRequest failed, error=" + std::to_string(GetLastError())); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return ""; }
    
    // Ignore SSL certificate errors
    if (isHttps) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                       | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_REVOCATION;
        InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }
    
    const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    
    BOOL bResult = HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers),
        (LPVOID)postData.c_str(), (DWORD)postData.length());
    
    if (bResult) {
        // Log HTTP status code
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL)) {
            LogInfo("HttpPost: HTTP status=" + std::to_string(statusCode) + " url=" + url);
        }
        
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result.append(buffer, bytesRead);
            bytesRead = 0;
        }
        LogInfo("HttpPost: response=" + (result.empty() ? "(empty)" : result.substr(0, 200)));
    } else {
        DWORD err = GetLastError();
        LogError("HttpPost: HttpSendRequest failed, error=" + std::to_string(err));
        // Retry once after ignoring SSL errors
        if (err == ERROR_INTERNET_INVALID_CA || err == 12045) {
            DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                           | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_REVOCATION;
            InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            bResult = HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers),
                (LPVOID)postData.c_str(), (DWORD)postData.length());
            if (bResult) {
                char buffer[4096];
                DWORD bytesRead = 0;
                while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    result.append(buffer, bytesRead);
                    bytesRead = 0;
                }
                LogInfo("HttpPost: retry response=" + (result.empty() ? "(empty)" : result.substr(0, 200)));
            } else {
                LogError("HttpPost: retry also failed, error=" + std::to_string(GetLastError()));
            }
        }
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return result;
}
#else
// macOS: use the system curl binary (always present) for HTTPS POST
std::string CYouTubeSource::HttpPost(const std::string& url, const std::string& postData)
{
    auto shellEscape = [](const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\'') out += "'\\''";
            else out += c;
        }
        return out;
    };
    
    std::string cmd = "curl -s -m 15 -X POST"
                      " -H 'Content-Type: application/x-www-form-urlencoded'"
                      " -A 'YouTubeSource/1.0'"
                      " --data '" + shellEscape(postData) + "'"
                      " '" + shellEscape(url) + "' 2>/dev/null";
    
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) { LogError("HttpPost: popen(curl) failed"); return ""; }
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), pipe)) > 0) result.append(buffer, n);
    pclose(pipe);
    LogInfo("HttpPost: response=" + (result.empty() ? "(empty)" : result.substr(0, 200)));
    return result;
}
#endif

bool CYouTubeSource::ValidateLicenseOnline()
{
    if (LicenseServerURL.empty() || LicenseKey.empty()) return false;
    
    std::string url = LicenseServerURL + "/pages/api/plugin-validate.php";
    std::string postData = "key=" + LicenseKey + "&machine_id=" + MachineID + "&plugin=youtube-source";
    
    LogInfo("Validating license online: " + url);
    std::string response = HttpPost(url, postData);
    
    if (response.empty()) {
        LogWarning("Online validation failed (no response), using local cache");
        return false;
    }
    
    LogInfo("Validate response: " + response);
    
    // Simple JSON parsing for status and expiry
    auto getJsonValue = [](const std::string& json, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };
    
    std::string status = getJsonValue(response, "status");
    std::string expiry = getJsonValue(response, "expiry");
    std::string serverTime = getJsonValue(response, "server_time");
    
    // Anti-clock-tamper: always update last known time from server (trusted source)
    if (!serverTime.empty()) {
        LogInfo("Server time received: " + serverTime);
        if (LastKnownTime.empty() || serverTime > LastKnownTime) {
            LastKnownTime = serverTime;
        }
    }
    
    if (status == "active") {
        // Update local cache with server expiry
        if (!expiry.empty()) {
            ExpiryDate = expiry.substr(0, 10);  // YYYY-MM-DD
        }
        SaveLicense();
        LogInfo("Online validation: ACTIVE, expires " + ExpiryDate);
        return true;
    }
    else if (status == "expired") {
        LogWarning("Online validation: License EXPIRED");
        ExpiryDate = expiry.substr(0, 10);
        SaveLicense();
        return false;
    }
    else if (status == "revoked") {
        LogWarning("Online validation: License REVOKED");
        LicenseKey = "";
        ExpiryDate = "";
        SaveLicense();
        return false;
    }
    else if (status == "machine_mismatch") {
        LogWarning("Online validation: Machine mismatch - key bound to different machine");
        LicenseKey = "";
        ExpiryDate = "";
        SaveLicense();
        return false;
    }
    
    LogWarning("Online validation: Unknown status: " + status);
    return false;
}

bool CYouTubeSource::ActivateLicenseOnline(const std::string& key)
{
    if (LicenseServerURL.empty()) {
        LogWarning("No license server URL configured");
        return false;
    }
    
    // Validate key format: SC-XXXX-XXXX-XXXX-XXXX-XXXX (or TM-, YT-, etc.)
    std::regex keyPattern(R"([A-Z]{2}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4})");
    if (!std::regex_match(key, keyPattern)) {
        LogWarning("Invalid key format for online activation: " + key);
        return false;
    }
    
    std::string url = LicenseServerURL + "/pages/api/plugin-activate.php";
    std::string computerName = GetComputerName();
    std::string postData = "key=" + key + "&machine_id=" + MachineID + "&computer_name=" + computerName + "&plugin=youtube-source";
    
    LogInfo("Activating license online: " + url);
    std::string response = HttpPost(url, postData);
    
    if (response.empty()) {
        LogError("Online activation failed (no response from server)");
        return false;
    }
    
    LogInfo("Activate response: " + response);
    
    // Simple JSON parsing
    auto getJsonValue = [](const std::string& json, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };
    
    std::string status = getJsonValue(response, "status");
    std::string expiry = getJsonValue(response, "expiry");
    std::string message = getJsonValue(response, "message");
    std::string serverTime = getJsonValue(response, "server_time");
    
    // Anti-clock-tamper: update last known time from server
    if (!serverTime.empty()) {
        LogInfo("Server time received (activate): " + serverTime);
        if (LastKnownTime.empty() || serverTime > LastKnownTime) {
            LastKnownTime = serverTime;
        }
    }
    
    if (status == "active") {
        LicenseKey = key;
        ExpiryDate = expiry.empty() ? "" : expiry.substr(0, 10);  // YYYY-MM-DD or empty for lifetime
        IsLicensed = true;
        SaveLicense();
        LogInfo("Online activation SUCCESS: " + key + (ExpiryDate.empty() ? " (lifetime)" : " expires " + ExpiryDate));
        return true;
    }
    else if (status == "max_devices") {
        LogWarning("Online activation FAILED: Maximum device activations reached - " + message);
        return false;
    }
    
    LogWarning("Online activation FAILED: " + status + " - " + message);
    return false;
}

//////////////////////////////////////////////////////////////////////////
// Marketplace account integration

bool CYouTubeSource::LoginToMarketplace(const std::string& email, const std::string& password, bool savePassword)
{
    if (LicenseServerURL.empty()) {
        LogWarning("No license server URL configured");
        return false;
    }
    
    std::string url = LicenseServerURL + "/pages/api/login";
    std::string postData = "email=" + email + "&password=" + password + "&plugin=youtube-source&device_name=" + GetComputerName();
    
    LogInfo("Attempting marketplace login for: " + email);
    std::string response = HttpPost(url, postData);
    
    if (response.empty()) {
        LogError("Login failed (no response from server)");
        return false;
    }
    
    LogInfo("Login response received, length: " + std::to_string(response.length()));
    LogInfo("Login response: " + response);
    
    // Parse JSON response
    auto getJsonValue = [](const std::string& json, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };
    
    auto getJsonInt = [](const std::string& json, const std::string& key) -> int {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return 0;
        pos += search.length();
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) return 0;
        return std::stoi(json.substr(pos, end - pos));
    };
    
    std::string status = getJsonValue(response, "status");
    
    if (status == "success" || status == "active") {
        IsLoggedIn = true;
        AccountEmail = email;
        AccountName = getJsonValue(response, "name");
        AuthToken = getJsonValue(response, "token");
        
        // Parse license key and expiry from response
        LicenseKey = getJsonValue(response, "license_key");
        ExpiryDate = getJsonValue(response, "expires");
        LogInfo("Parsed from login response: LicenseKey=" + (LicenseKey.empty() ? "(empty)" : LicenseKey) + 
                ", ExpiryDate=" + (ExpiryDate.empty() ? "(empty)" : ExpiryDate));
        
        // Parse enhanced license fields
        TokenExpiry = getJsonValue(response, "token_expires");
        LicenseType = getJsonValue(response, "license_type");
        LicenseMessage = getJsonValue(response, "message");
        PluginId = getJsonInt(response, "plugin_id");
        MaxActivations = getJsonInt(response, "max_activations");
        CurrentActivations = getJsonInt(response, "current_activations");
        MaxSubLicenses = getJsonInt(response, "max_sub_licenses");
        CurrentSubLicenses = getJsonInt(response, "current_sub_licenses");
        LogInfo("Parsed license details: Type=" + LicenseType + ", TokenExpiry=" + TokenExpiry + 
                ", Activations=" + std::to_string(CurrentActivations) + "/" + std::to_string(MaxActivations));
        
        // Parse sub-license info
        std::string isSharedStr = getJsonValue(response, "is_shared");
        IsSharedLicense = (isSharedStr == "true" || isSharedStr == "1" || response.find("\"is_shared\":true") != std::string::npos);
        
        // Parse nested shared_info object if present
        if (IsSharedLicense) {
            size_t sharedInfoPos = response.find("\"shared_info\":");
            if (sharedInfoPos != std::string::npos) {
                std::string sharedInfoJson = response.substr(sharedInfoPos);
                SharedOwnerName = getJsonValue(sharedInfoJson, "owner_name");
                SharedOwnerEmail = getJsonValue(sharedInfoJson, "owner_email");
                SharedDate = getJsonValue(sharedInfoJson, "shared_date");
            }
        }
        
        // Update license status
        LicStatus = LIC_VALID;
        IsLicensed = true;
        
        // Save password preference and encrypt password if requested
        SavePasswordEnabled = savePassword;
        if (savePassword) {
            SavedPassword = EncryptPassword(password);
        } else {
            SavedPassword = "";
        }
        
        SaveLicense();
        
        // Notify all listeners (search dialog, etc.) that license status changed
        UpdateLicenseStatus(true);
        
        LogInfo("Marketplace login SUCCESS for: " + email);
        return true;
    }
    
    LogWarning("Marketplace login FAILED: " + status);
    return false;
}

bool CYouTubeSource::LoginToMarketplace(const std::string& email, const std::string& password)
{
    return LoginToMarketplace(email, password, SavePasswordEnabled);
}

void CYouTubeSource::LogoutFromMarketplace()
{
    IsLoggedIn = false;
    IsLicensed = false;
    LicStatus = LIC_UNCHECKED;
    AccountEmail = "";
    AccountName = "";
    AuthToken = "";
    LicenseKey = "";
    ExpiryDate = "";
    CurrentActivations = 0;
    MaxActivations = 0;
    
    SaveLicense();
    
    // Notify all listeners (search dialog, etc.) that license status changed
    UpdateLicenseStatus(false);
    
    LogInfo("Logged out from marketplace");
}

void CYouTubeSource::OpenMarketplaceDashboard()
{
    std::string url = LicenseServerURL + "/dashboard";
    LogInfo("Opening marketplace dashboard: " + url);
    OpenUrlInBrowser(url);
}

//////////////////////////////////////////////////////////////////////////
// Auto-update system

void CYouTubeSource::CheckForUpdates()
{
    // Version check disabled - marketplace doesn't have version API yet
    return;
    
    if (LicenseServerURL.empty()) return;

    std::string url = LicenseServerURL + "/pages/api/version.php";
    std::string response = HttpPost(url, "check=1");
    if (response.empty()) { LogInfo("Version check: no response"); return; }

    LogInfo("Version check: " + response.substr(0, 200));

    auto getStr = [](const std::string& json, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.length();
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos >= json.size() || json[pos] != '"') return "";
        pos++;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                if (json[pos] == 'n') val += '\n';
                else val += json[pos];
            } else { val += json[pos]; }
            pos++;
        }
        return val;
    };

    auto getInt = [](const std::string& json, const std::string& key) -> int {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return 0;
        pos += search.length();
        while (pos < json.size() && json[pos] == ' ') pos++;
        size_t end = pos;
        while (end < json.size() && isdigit((unsigned char)json[end])) end++;
        if (end == pos) return 0;
        try { return std::stoi(json.substr(pos, end - pos)); } catch (...) { return 0; }
    };

    int serverCode = getInt(response, "version_code");
    if (serverCode > PLUGIN_VERSION_CODE) {
        UpdateAvailable      = true;
        UpdateNewVersion     = getStr(response, "version");
        UpdateChangelog      = getStr(response, "changelog");
        UpdateDownloadUrl64  = getStr(response, "download_url_64");
        UpdateDownloadUrl32  = getStr(response, "download_url_32");
        LogInfo("Update available: v" + UpdateNewVersion + " (current: " PLUGIN_VERSION ")");
    } else {
        LogInfo("Plugin up to date (v" PLUGIN_VERSION ")");
    }
}

#ifdef _WIN32
bool CYouTubeSource::HttpDownloadFile(const std::string& url, const std::string& localPath)
{
    std::string host, path;
    bool isHttps = false;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;

    std::string u = url;
    if (u.substr(0, 8) == "https://") { isHttps = true; port = INTERNET_DEFAULT_HTTPS_PORT; u = u.substr(8); }
    else if (u.substr(0, 7) == "http://") { u = u.substr(7); }

    size_t sl = u.find('/');
    if (sl != std::string::npos) { host = u.substr(0, sl); path = u.substr(sl); }
    else { host = u; path = "/"; }

    HINTERNET hNet = InternetOpenA("YouTubeSource/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return false;
    DWORD to = 60000;
    InternetSetOptionA(hNet, INTERNET_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    InternetSetOptionA(hNet, INTERNET_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (isHttps) flags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = InternetConnectA(hNet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hNet); return false; }

    HINTERNET hReq = HttpOpenRequestA(hConn, "GET", path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hReq) { InternetCloseHandle(hConn); InternetCloseHandle(hNet); return false; }

    if (isHttps) {
        DWORD sf = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                 | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_REVOCATION;
        InternetSetOptionA(hReq, INTERNET_OPTION_SECURITY_FLAGS, &sf, sizeof(sf));
    }

    if (!HttpSendRequestA(hReq, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hNet);
        return false;
    }

    std::ofstream out(localPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) { InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hNet); return false; }

    char buf[8192]; DWORD rd = 0, total = 0;
    while (InternetReadFile(hReq, buf, sizeof(buf), &rd) && rd > 0) { out.write(buf, rd); total += rd; }
    out.close();
    InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hNet);
    LogInfo("HttpDownloadFile: downloaded " + std::to_string(total) + " bytes to " + localPath);
    return total > 50000;
}

void CYouTubeSource::PerformUpdate()
{
#ifdef _WIN64
    std::string dlUrl  = UpdateDownloadUrl64;
    std::string dll    = "YouTubeSource64.dll";
#else
    std::string dlUrl  = UpdateDownloadUrl32;
    std::string dll    = "YouTubeSource32.dll";
#endif
    if (dlUrl.empty()) {
        MessageBoxA(NULL, "No download URL. Contact support.", "Update Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::string dir     = GetModuleDirectory();
    std::string curDll  = dir + "\\" + dll;
    std::string tmpDll  = dir + "\\" + dll + ".new";
    std::string batPath = dir + "\\yt_update.bat";

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    bool ok = HttpDownloadFile(dlUrl, tmpDll);
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    if (!ok) {
        try { std::filesystem::remove(tmpDll); } catch (...) {}
        MessageBoxA(NULL, "Download failed. Check your internet connection.", "Update Failed", MB_OK | MB_ICONERROR);
        return;
    }

    // Find VirtualDJ.exe from running process list
    std::string vdjExe;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe = { sizeof(pe) };
        if (Process32First(hSnap, &pe)) {
            do {
                char narrow[MAX_PATH] = {};
                WideCharToMultiByte(CP_ACP, 0, pe.szExeFile, -1, narrow, MAX_PATH, NULL, NULL);
                std::string n = narrow;
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (n == "virtualdj.exe") {
                    HANDLE hp = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                    if (hp) {
                        char buf[MAX_PATH] = {}; DWORD sz = MAX_PATH;
                        if (QueryFullProcessImageNameA(hp, 0, buf, &sz)) vdjExe = buf;
                        CloseHandle(hp);
                    }
                    break;
                }
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    if (vdjExe.empty()) {
        for (const char* p : { "C:\\Program Files\\VirtualDJ\\VirtualDJ.exe",
                                "C:\\Program Files (x86)\\VirtualDJ\\VirtualDJ.exe" })
            if (std::filesystem::exists(p)) { vdjExe = p; break; }
    }

    // Write install batch script
    {
        std::ofstream bat(batPath);
        bat << "@echo off\n";
        bat << "timeout /t 4 /nobreak > nul\n";
        bat << "taskkill /IM VirtualDJ.exe /F > nul 2>&1\n";
        bat << "timeout /t 2 /nobreak > nul\n";
        bat << "copy /Y \"" << tmpDll << "\" \"" << curDll << "\"\n";
        bat << "del \"" << tmpDll << "\"\n";
        if (!vdjExe.empty()) bat << "start \"\" \"" << vdjExe << "\"\n";
        bat << "del \"%~f0\"\n";
    }

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    std::string cmd = "cmd /c \"" + batPath + "\"";
    char* cbuf = _strdup(cmd.c_str());

    if (CreateProcessA(NULL, cbuf, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread); free(cbuf);
        MessageBoxA(NULL,
            ("v" + UpdateNewVersion + " downloaded!\n\nClick OK and VirtualDJ will close and restart automatically.").c_str(),
            "Update Ready", MB_OK | MB_ICONINFORMATION);
        HWND vdjWnd = FindWindowA("VDJApp", NULL);
        if (!vdjWnd) vdjWnd = FindWindowA(NULL, "VirtualDJ");
        if (vdjWnd) PostMessageA(vdjWnd, WM_CLOSE, 0, 0);
        else ShellExecuteA(NULL, "open", "taskkill.exe", "/IM VirtualDJ.exe", NULL, SW_HIDE);
    } else {
        free(cbuf);
        MessageBoxA(NULL, "Failed to launch installer. Please install manually.", "Update Error", MB_OK | MB_ICONERROR);
    }
}
#else
// macOS: plugin self-update not supported (updates ship via installer)
bool CYouTubeSource::HttpDownloadFile(const std::string& url, const std::string& localPath)
{
    auto shellEscape = [](const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\'') out += "'\\''";
            else out += c;
        }
        return out;
    };
    std::string cmd = "curl -s -L -m 120 -o '" + shellEscape(localPath) + "' '" + shellEscape(url) + "' 2>/dev/null";
    int rc = system(cmd.c_str());
    if (rc != 0) return false;
    std::error_code ec;
    auto size = std::filesystem::file_size(localPath, ec);
    return !ec && size > 50000;
}

void CYouTubeSource::PerformUpdate()
{
    LogWarning("PerformUpdate: in-place self-update is not supported on macOS");
}
#endif

//////////////////////////////////////////////////////////////////////////
// Background License Validation

void CYouTubeSource::CheckLicenseBackground()
{
    if (ValidationInProgress) {
        LogInfo("License validation already in progress, skipping");
        return;
    }
    
    ValidationInProgress = true;
    
    if (LicenseValidationThread.joinable()) {
        LicenseValidationThread.detach();
    }
    
    LicenseValidationThread = std::thread([this]() {
        LogInfo("Background license validation started");
        bool result = CheckLicense();
        UpdateLicenseStatus(result);
        ValidationInProgress = false;
        LogInfo("Background license validation completed: " + std::string(result ? "VALID" : "INVALID"));
    });
}

void CYouTubeSource::StartLicenseTimer()
{
    StopTimer = false;
    
    if (LicenseTimerThread.joinable()) {
        LicenseTimerThread.detach();
    }
    
    LicenseTimerThread = std::thread([this]() {
        LogInfo("License validation timer started (15 second interval)");
        
        while (!StopTimer) {
            // Wait 15 seconds
            for (int i = 0; i < 150 && !StopTimer; i++) {
                Sleep(100); // Check stop flag every 100ms
            }
            
            if (!StopTimer) {
                LogInfo("Timer: Running periodic license validation");
                CheckLicenseBackground();
            }
        }
        
        LogInfo("License validation timer stopped");
    });
}

void CYouTubeSource::StopLicenseTimer()
{
    StopTimer = true;
    if (LicenseTimerThread.joinable()) {
        LicenseTimerThread.join();
    }
}

void CYouTubeSource::UpdateLicenseStatus(bool licensed)
{
    bool wasLicensed = IsLicensed;
    IsLicensed = licensed;
    
    if (wasLicensed != licensed) {
        LogInfo("License status changed: " + std::string(wasLicensed ? "LICENSED" : "UNLICENSED") + " -> " + std::string(licensed ? "LICENSED" : "UNLICENSED"));
    }
    
    // Always notify listeners (like search dialog) to ensure UI updates
    if (OnLicenseStatusChanged) {
        OnLicenseStatusChanged(licensed);
        LogInfo("Notified license status listeners: " + std::string(licensed ? "LICENSED" : "UNLICENSED"));
    }
    
    // Push to WebView2 UI
    PushLicenseToWeb();
    
#ifdef _WIN32
    // Update license dialog if it's open
    if (hLicenseDialogWindow) {
        PostMessageA(hLicenseDialogWindow, WM_USER + 100, licensed ? 1 : 0, 0);
    }
    
    // Update search dialog if it's open
    if (pSearchDialog) {
        pSearchDialog->SetLicenseStatus(licensed, 0, 1, ExpiryDate, MachineID);
        LogInfo("Updated search dialog license status: " + std::string(licensed ? "Pro" : "Unlicensed"));
    }
#endif
}

std::string CYouTubeSource::DecryptPassword(const std::string& encrypted)
{
    if (encrypted.empty()) return "";
    
    // Simple XOR decryption - same as encryption
    std::string decrypted;
    const char key[] = "YouTubeSource2024";
    int keyLen = strlen(key);
    
    for (size_t i = 0; i < encrypted.length(); i++) {
        decrypted += encrypted[i] ^ key[i % keyLen];
    }
    
    return decrypted;
}

std::string CYouTubeSource::EncryptPassword(const std::string& plain)
{
    if (plain.empty()) return "";
    
    // Simple XOR encryption
    std::string encrypted;
    const char key[] = "YouTubeSource2024";
    int keyLen = strlen(key);
    
    for (size_t i = 0; i < plain.length(); i++) {
        encrypted += plain[i] ^ key[i % keyLen];
    }
    
    // Convert to hex for storage
    std::string hex;
    char buf[3];
    for (char c : encrypted) {
        sprintf_s(buf, "%02X", (unsigned char)c);
        hex += buf;
    }
    
    return hex;
}

//////////////////////////////////////////////////////////////////////////
// DLL entry point

extern "C" VDJ_EXPORT HRESULT VDJ_API DllGetClassObject(const GUID &rclsid, const GUID &riid, void** ppObject)
{
#ifdef _WIN32
    if (rclsid == CLSID_VdjPlugin8) {
        if (riid == IID_IVdjPluginOnlineSource) {
#else
    if (memcmp(&rclsid, &CLSID_VdjPlugin8, sizeof(GUID)) == 0) {
        if (memcmp(&riid, &IID_IVdjPluginOnlineSource, sizeof(GUID)) == 0) {
#endif
            *ppObject = new CYouTubeSource();
            return S_OK;
        }
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}
