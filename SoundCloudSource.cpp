#include "SoundCloudSource.h"
#include "LicenseDialog.h"
#include "LicenseDialogV2.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <chrono>
#include <shlobj.h>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
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
#endif

//////////////////////////////////////////////////////////////////////////
// Plugin lifecycle

HRESULT VDJ_API CSoundCloudSource::OnLoad()
{
    InitializeLogging();
    LogInfo("SoundCloud Online Source Plugin loading...");
    
    // Find tools next to the DLL
    ToolsDir = GetModuleDirectory();
    ytDlpPath = ToolsDir + "\\yt-dlp.exe";
    ffmpegPath = ToolsDir + "\\ffmpeg.exe";
    ffmpegDir = ToolsDir;
    
    LogInfo("DLL directory: " + ToolsDir);
    LogInfo("yt-dlp: " + std::string(std::filesystem::exists(ytDlpPath) ? "FOUND" : "NOT FOUND"));
    LogInfo("ffmpeg: " + std::string(std::filesystem::exists(ffmpegPath) ? "FOUND" : "NOT FOUND"));
    
    // Create global cache directory
    std::filesystem::path cachePath;
    char path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path) == S_OK) {
        cachePath = std::filesystem::path(path) / "VirtualDJ" / "soundcloud-cloud";
    } else {
        cachePath = std::filesystem::temp_directory_path() / "SoundCloudSourceCache";
    }
    std::filesystem::create_directories(cachePath);
    GlobalCacheDir = cachePath.string();
    LogInfo("Cache directory: " + GlobalCacheDir);
    
    CleanupOldCache();
    
    // Load user settings (format preference, etc.)
    LoadSettings();
    
    // Generate machine ID for hardware binding
    MachineID = GetMachineID();
    LogInfo("Machine ID: " + MachineID);
    
    // Quick local license check (non-blocking)
    LoadLicense();
    if (IsLoggedIn && !AccountEmail.empty() && !AuthToken.empty()) {
        IsLicensed = true;
        LogInfo("Marketplace account login found: " + AccountEmail + " - treating as licensed");
    }
    
    // Start background validation (non-blocking)
    CheckLicenseBackground();
    
    // Start periodic license validation timer (every 15 seconds)
    StartLicenseTimer();
    
    // Initialize GDI+ for PNG loading
    Gdiplus::GdiplusStartupInput gdiplusInput;
    Gdiplus::GdiplusStartup(&GdiplusToken, &gdiplusInput, nullptr);
    
    // Check for updates in background (non-blocking)
    std::thread([this]() { CheckForUpdates(); }).detach();

    LogInfo("Plugin loaded successfully");
    return S_OK;
}

HRESULT VDJ_API CSoundCloudSource::OnGetPluginInfo(TVdjPluginInfo8 *infos)
{
    infos->PluginName = IsLicensed ? "SoundCloud Search - Pro v" PLUGIN_VERSION : "SoundCloud Search - Unlicensed v" PLUGIN_VERSION;
    infos->Author = "Virtual DJ Plugin";
    PluginDescription = IsLicensed
        ? "Search and play SoundCloud audio  |  Licensed until " + ExpiryDate
        : "Search and play SoundCloud audio  |  Trial: 5 free searches";
    infos->Description = PluginDescription.c_str();
    infos->Version = "2.0";
    infos->Flags = 0;
    
    // Load SoundCloud logo as bitmap from DLL resource (ID 7 = folder icon)
    if (!hPluginBitmap) {
        hPluginBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(7));
        if (hPluginBitmap) {
            LogInfo("Loaded SoundCloud logo from DLL resource ID 7");
        } else {
            LogWarning("Failed to load SoundCloud logo from DLL resource ID 7");
        }
    }
    infos->Bitmap = hPluginBitmap;
    
    return S_OK;
}

ULONG VDJ_API CSoundCloudSource::Release()
{
    LogInfo("Plugin releasing...");
    
    // Stop license validation timer
    StopLicenseTimer();
    
    SearchCancelled = true;
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
    
    delete this;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Search

HRESULT VDJ_API CSoundCloudSource::OnSearch(const char* search, IVdjTracksList* tracksList)
{
    if (!search || !tracksList) return E_POINTER;
    if (search[0] == '\0') return S_OK;
    
    std::string query(search);
    
    // Handle license activation via search box: type ACTIVATE:SC-XXXX-XXXX-XXXX
    if (query.rfind("ACTIVATE:", 0) == 0) {
        std::string key = query.substr(9);
        if (ActivateLicense(key)) {
            LogActivation(key, true);
            tracksList->add("LICENSE_OK", "License Activated!",
                            ("Valid until " + ExpiryDate).c_str(),
                            nullptr, nullptr, nullptr, "SoundCloud Source is now fully unlocked");
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
    
    // License check - require login for searches
    if (!IsLicensed) {
        LogWarning("Search attempted without license - opening License Manager");
        tracksList->add("LICENSE", "License Required",
                        "Please login or activate your license to search SoundCloud",
                        nullptr, nullptr, nullptr, "Click License Manager to continue");
        tracksList->finish();
        
        // Open License Manager for login/activation
        ShowLicenseDialog();
        return S_OK;
    }
    
    if (query.empty()) return S_OK;
    
    LogInfo("OnSearch called: \"" + query + "\"");
    SearchCancelled = false;
    
    // Use yt-dlp to search SoundCloud
    // yt-dlp "scsearch25:query" --print "%(id)s|%(title)s|%(uploader)s|%(duration)s" --flat-playlist --no-warnings
    std::string tempDir = GlobalCacheDir;
    std::string outputFile = (std::filesystem::path(tempDir) / "search_output.txt").string();
    std::string batFile = (std::filesystem::path(tempDir) / "search.bat").string();
    
    {
        std::ofstream bat(batFile);
        bat << "@echo off" << std::endl;
        bat << "\"" << ytDlpPath << "\" --no-warnings --flat-playlist --print \"%%(id)s|%%(title)s|%%(uploader)s|%%(duration_string)s|%%(webpage_url)s\" \"scsearch25:" << query << "\" > \"" << outputFile << "\" 2>&1" << std::endl;
        bat.close();
    }
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    std::string cmdLine = "cmd /c \"" + batFile + "\"";
    char* cmdLineBuf = _strdup(cmdLine.c_str());
    
    if (!CreateProcessA(NULL, cmdLineBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        free(cmdLineBuf);
        LogError("Failed to start search process");
        return E_FAIL;
    }
    free(cmdLineBuf);
    
    // Wait for search to complete, checking for cancellation
    while (WaitForSingleObject(pi.hProcess, 200) == WAIT_TIMEOUT) {
        if (SearchCancelled) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            LogInfo("Search cancelled");
            return S_OK;
        }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // Parse results
    std::ifstream inFile(outputFile);
    std::string line;
    int count = 0;
    
    while (std::getline(inFile, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line.find("WARNING:") == 0 || line.find("ERROR:") == 0) continue;
        
        // Parse: id|title|uploader|duration|url
        size_t p1 = line.find('|');
        if (p1 == std::string::npos) continue;
        size_t p2 = line.find('|', p1 + 1);
        if (p2 == std::string::npos) continue;
        size_t p3 = line.find('|', p2 + 1);
        if (p3 == std::string::npos) continue;
        size_t p4 = line.find('|', p3 + 1);
        if (p4 == std::string::npos) continue;
        
        std::string videoId = line.substr(0, p1);
        std::string title = line.substr(p1 + 1, p2 - p1 - 1);
        std::string artist = line.substr(p2 + 1, p3 - p2 - 1);
        std::string durationStr = line.substr(p3 + 1, p4 - p3 - 1);
        std::string trackUrl = line.substr(p4 + 1);
        
        // Use the actual SoundCloud URL from search results as the uniqueId
        std::string coverUrl = "";
        
        // Parse duration string (e.g. "3:45" or "1:02:30") to seconds
        float durationSec = 0;
        std::istringstream dss(durationStr);
        std::string part;
        std::vector<int> parts;
        while (std::getline(dss, part, ':')) {
            try { parts.push_back(std::stoi(part)); } catch (...) {}
        }
        if (parts.size() == 3) durationSec = (float)(parts[0] * 3600 + parts[1] * 60 + parts[2]);
        else if (parts.size() == 2) durationSec = (float)(parts[0] * 60 + parts[1]);
        else if (parts.size() == 1) durationSec = (float)parts[0];
        
        bool isVideoFormat = (DownloadFormat == "mp4");
        
        tracksList->add(
            trackUrl.c_str(),   // uniqueId - use full URL instead of numeric ID
            title.c_str(),      // title
            artist.c_str(),     // artist
            0,                  // remix
            0,                  // genre
            0,                  // label
            durationStr.c_str(),// comment (show duration here)
            coverUrl.c_str(),   // coverUrl (YouTube thumbnail)
            0,                  // streamUrl (provided on demand via GetStreamUrl)
            durationSec,        // length
            0,                  // bpm
            0,                  // key
            0,                  // year
            isVideoFormat,      // isVideo
            false               // isKaraoke
        );
        count++;
        LogDebug("  Result: [" + videoId + "] " + title + " - " + artist + " (" + durationStr + ")");
    }
    inFile.close();
    
    // Cleanup temp files
    try {
        std::filesystem::remove(batFile);
        std::filesystem::remove(outputFile);
    } catch (...) {}
    
    tracksList->finish();
    LogInfo("Search returned " + std::to_string(count) + " results");
    return S_OK;
}

HRESULT VDJ_API CSoundCloudSource::OnSearchCancel()
{
    LogInfo("Search cancel requested");
    SearchCancelled = true;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Stream URL

HRESULT VDJ_API CSoundCloudSource::GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage)
{
    if (!uniqueId) return E_POINTER;
    
    std::string videoId(uniqueId);
    LogInfo("GetStreamUrl called for: " + videoId);
    
    // Skip special IDs (license messages etc)
    if (videoId.find("LICENSE") == 0) return E_FAIL;
    
    // For SoundCloud, use the full permalink URL directly
    std::string videoUrl = videoId;
    std::string cacheId = videoId;
    
    // Extract a clean cache ID for filename from the URL
    // For URLs like "https://soundcloud.com/artist/track-name", extract "track-name"
    if (videoId.find("soundcloud.com/") != std::string::npos) {
        size_t lastSlash = videoId.rfind('/');
        if (lastSlash != std::string::npos) {
            cacheId = videoId.substr(lastSlash + 1);
        }
    }
    
    // Check if we already have a cached file for this video (exact match by cacheId)
    std::string ext = (DownloadFormat == "mp4") ? ".mp4" : ".mp3";
    std::string cachedPath = (std::filesystem::path(GlobalCacheDir) / (cacheId + ext)).string();
    if (std::filesystem::exists(cachedPath)) {
        LogInfo("GetStreamUrl: cache hit: " + cachedPath);
        url = cachedPath.c_str();
        return S_OK;
    }
    
    // Strategy 1: Get direct stream URL from yt-dlp (fast, ~1-2 seconds)
    {
        std::string streamOutputFile = (std::filesystem::path(GlobalCacheDir) / "stream_url.txt").string();
        std::string streamBatFile = (std::filesystem::path(GlobalCacheDir) / "stream.bat").string();
        
        std::string formatArg;
        if (DownloadFormat == "mp4") {
            // Use single combined URL for streaming - split video+audio returns 2 URLs VDJ can't handle
            formatArg = "\"best[ext=mp4]/best\"";
        } else {
            formatArg = "\"bestaudio\"";
        }
        
        {
            std::ofstream bat(streamBatFile);
            bat << "@echo off" << std::endl;
            bat << "\"" << ytDlpPath << "\" --no-warnings --get-url -f " << formatArg << " \"" << videoUrl << "\" > \"" << streamOutputFile << "\" 2>&1" << std::endl;
            bat.close();
        }
        
        std::vector<std::string> output;
        RunCommand("cmd /c \"" + streamBatFile + "\"", output);
        
        // Read the direct URL(s)
        std::string directUrl;
        {
            std::ifstream sf(streamOutputFile);
            std::string sline;
            while (std::getline(sf, sline)) {
                while (!sline.empty() && (sline.back() == '\r' || sline.back() == '\n')) sline.pop_back();
                if (!sline.empty() && sline.find("http") == 0) {
                    directUrl = sline;
                    break;  // Use first URL (audio for mp3, video for mp4)
                }
            }
        }
        try { std::filesystem::remove(streamBatFile); std::filesystem::remove(streamOutputFile); } catch (...) {}
        
        if (!directUrl.empty()) {
            LogInfo("GetStreamUrl: returning direct stream URL (" + std::to_string(directUrl.length()) + " chars)");
            url = directUrl.c_str();
            return S_OK;
        }
        LogWarning("GetStreamUrl: direct URL extraction failed, falling back to download");
    }
    
    // Strategy 2: Download and cache the file (slower, but reliable)
    // Get title via yt-dlp
    std::string titleOutputFile = (std::filesystem::path(GlobalCacheDir) / "title_output.txt").string();
    std::string titleBatFile = (std::filesystem::path(GlobalCacheDir) / "title.bat").string();
    
    {
        std::ofstream bat(titleBatFile);
        bat << "@echo off" << std::endl;
        bat << "\"" << ytDlpPath << "\" --no-warnings --print \"%%(title)s\" \"" << videoUrl << "\" > \"" << titleOutputFile << "\" 2>&1" << std::endl;
        bat.close();
    }
    
    std::vector<std::string> titleOutput;
    RunCommand("cmd /c \"" + titleBatFile + "\"", titleOutput);
    
    // Read title from output file
    std::string title = "SoundCloud_" + videoId;
    {
        std::ifstream tf(titleOutputFile);
        std::string tline;
        while (std::getline(tf, tline)) {
            while (!tline.empty() && (tline.back() == '\r' || tline.back() == '\n')) tline.pop_back();
            if (!tline.empty() && tline.find("WARNING:") != 0 && tline.find("ERROR:") != 0) {
                title = tline;
                break;
            }
        }
    }
    try { std::filesystem::remove(titleBatFile); std::filesystem::remove(titleOutputFile); } catch (...) {}
    
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

std::string CSoundCloudSource::DownloadAndCacheTrack(const std::string& videoId, const std::string& title)
{
    std::string ext = (DownloadFormat == "mp4") ? ".mp4" : ".mp3";
    
    // For SoundCloud, use the full permalink URL directly
    std::string videoUrl = videoId;
    std::string cacheId = videoId;
    
    // Extract a clean cache ID for filename from the URL
    // For URLs like "https://soundcloud.com/artist/track-name", extract "track-name"
    if (videoId.find("soundcloud.com/") != std::string::npos) {
        size_t lastSlash = videoId.rfind('/');
        if (lastSlash != std::string::npos) {
            cacheId = videoId.substr(lastSlash + 1);
        }
    }
    
    std::string cacheFile = (std::filesystem::path(GlobalCacheDir) / (cacheId + ext)).string();
    
    // Check cache first
    if (std::filesystem::exists(cacheFile)) {
        LogInfo("Cache hit: " + cacheFile);
        UpdateDownloadProgress(title, "Cached", 100.0f, "", "");
        return cacheFile;
    }
    
    // Show download starting
    UpdateDownloadProgress(title, "Downloading", 5.0f, "", "");
    std::string command;
    if (DownloadFormat == "mp4") {
        // Try 1080p first, then 720p, then best available
        command = "\"" + ytDlpPath + "\" --no-warnings --newline -f \"(bestvideo[height<=1080][ext=mp4]+bestaudio[ext=m4a])/(bestvideo[height<=720][ext=mp4]+bestaudio[ext=m4a])/best[ext=mp4]/best\" --merge-output-format mp4 --ffmpeg-location \"" + ffmpegDir + "\" --add-metadata -o \"" + cacheFile + "\" \"" + videoUrl + "\"";
    } else {
        command = "\"" + ytDlpPath + "\" --no-warnings --newline --extract-audio --audio-format mp3 --ffmpeg-location \"" + ffmpegDir + "\" --add-metadata -o \"" + cacheFile + "\" \"" + videoUrl + "\"";
    }
    
    std::string batFile = (std::filesystem::path(GlobalCacheDir) / "download.bat").string();
    std::string outFile = (std::filesystem::path(GlobalCacheDir) / "download_output.txt").string();
    
    {
        std::ofstream bat(batFile);
        bat << "@echo off" << std::endl;
        bat << command << " > \"" << outFile << "\" 2>&1" << std::endl;
        bat.close();
    }
    
    // Run download process
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    std::string cmdLine = "cmd /c \"" + batFile + "\"";
    char* cmdBuf = _strdup(cmdLine.c_str());
    
    bool processStarted = false;
    if (CreateProcessA(NULL, cmdBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        processStarted = true;
        
        // Poll output file for progress while process runs
        DWORD waitResult;
        int pollCount = 0;
        do {
            waitResult = WaitForSingleObject(pi.hProcess, 500); // Check every 500ms
            pollCount++;
            
            // Parse output file for progress info
            std::ifstream progressFile(outFile);
            std::string lastLine;
            std::string line;
            while (std::getline(progressFile, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
                if (!line.empty()) lastLine = line;
            }
            progressFile.close();
            
            if (!lastLine.empty()) {
                // yt-dlp progress: [download]  45.2% of 4.52MiB at 2.31MiB/s ETA 00:02
                float pct = 0;
                std::string speed, eta;
                
                size_t pctPos = lastLine.find('%');
                if (pctPos != std::string::npos) {
                    // Find start of percentage number
                    size_t numStart = pctPos;
                    while (numStart > 0 && (isdigit(lastLine[numStart-1]) || lastLine[numStart-1] == '.')) numStart--;
                    try { pct = std::stof(lastLine.substr(numStart, pctPos - numStart)); } catch (...) {}
                }
                
                size_t atPos = lastLine.find(" at ");
                if (atPos != std::string::npos) {
                    size_t speedEnd = lastLine.find(' ', atPos + 4);
                    if (speedEnd == std::string::npos) speedEnd = lastLine.size();
                    speed = lastLine.substr(atPos + 4, speedEnd - atPos - 4);
                }
                
                size_t etaPos = lastLine.find("ETA ");
                if (etaPos != std::string::npos) {
                    eta = lastLine.substr(etaPos + 4);
                    // Trim whitespace
                    while (!eta.empty() && (eta.back() == ' ' || eta.back() == '\r')) eta.pop_back();
                }
                
                // Detect conversion phase
                std::string phase = "Downloading";
                if (lastLine.find("[ExtractAudio]") != std::string::npos || 
                    lastLine.find("[Merger]") != std::string::npos ||
                    lastLine.find("Deleting original") != std::string::npos) {
                    phase = (DownloadFormat == "mp4") ? "Merging MP4" : "Converting to MP3";
                    pct = 90.0f;
                } else if (lastLine.find("[Metadata]") != std::string::npos) {
                    phase = "Adding metadata";
                    pct = 95.0f;
                } else if (pct > 0) {
                    // Scale download to 0-85% range
                    pct = pct * 0.85f;
                }
                
                if (pct > 0 || !phase.empty()) {
                    UpdateDownloadProgress(title, phase, pct, speed, eta);
                }
            }
            
        } while (waitResult == WAIT_TIMEOUT && pollCount < 240); // Max 2 min
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        // Give the file system a moment to flush the file to disk
        Sleep(1000);
    }
    free(cmdBuf);
    
    try { std::filesystem::remove(batFile); std::filesystem::remove(outFile); } catch (...) {}
    
    if (std::filesystem::exists(cacheFile)) {
        LogInfo("Download complete: " + cacheFile);
        UpdateDownloadProgress(title, "Complete", 100.0f, "", "");
        return cacheFile;
    }
    
    LogError("Download failed, cache file not found: " + cacheFile);
    UpdateDownloadProgress(title, "Error", 0.0f, "", "");
    return "";
}

void CSoundCloudSource::UpdateDownloadProgress(const std::string& title, const std::string& phase, float percent, const std::string& speed, const std::string& eta)
{
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
}

//////////////////////////////////////////////////////////////////////////
// IVdjPluginOnlineSource: Context Menus

HRESULT VDJ_API CSoundCloudSource::GetFolderContextMenu(const char* folderUniqueId, IVdjContextMenu* contextMenu)
{
    if (!contextMenu) return E_POINTER;
    LogInfo("GetFolderContextMenu called for: " + std::string(folderUniqueId ? folderUniqueId : "(root)"));
    
    // Add menu items
    contextMenu->add("Open Advanced Search Window");
    contextMenu->add("Manage License");
    
    LogInfo("Added 2 menu items to context menu");
    return S_OK;
}

HRESULT VDJ_API CSoundCloudSource::OnFolderContextMenu(const char* folderUniqueId, size_t menuIndex)
{
    LogInfo("OnFolderContextMenu: index=" + std::to_string(menuIndex));
    
    if (menuIndex == 0) {
        OpenSearchWindow();
    } else if (menuIndex == 1) {
        ShowLicenseDialog();
    }
    return S_OK;
}

HRESULT VDJ_API CSoundCloudSource::GetContextMenu(const char* uniqueId, IVdjContextMenu* contextMenu)
{
    if (!contextMenu || !uniqueId) return E_POINTER;
    LogInfo("GetContextMenu called for track: " + std::string(uniqueId));
    
    contextMenu->add("Open in Browser");
    return S_OK;
}

HRESULT VDJ_API CSoundCloudSource::OnContextMenu(const char* uniqueId, size_t menuIndex)
{
    if (!uniqueId) return E_POINTER;
    LogInfo("OnContextMenu: uniqueId=" + std::string(uniqueId) + ", index=" + std::to_string(menuIndex));
    
    if (menuIndex == 0) {
        // Open the SoundCloud track in the default browser
        std::string url = "https://soundcloud.com/" + std::string(uniqueId);
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// Search Dialog (Advanced Search Window)

void CSoundCloudSource::OpenSearchWindow()
{
    LogInfo("Opening advanced search window...");
    
    // Refresh license status from server
    LogInfo("Refreshing license status from server...");
    CheckLicenseBackground();
    
    LogInfo("Current license state: IsLicensed=" + std::string(IsLicensed ? "true" : "false") + 
            " Key=" + LicenseKey + " Expiry=" + ExpiryDate);
    
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
        
        // Set up license change callback to update UI when license status changes
        OnLicenseStatusChanged = [this](bool licensed) {
            if (pSearchDialog) {
                pSearchDialog->SetLicenseStatus(licensed, FreeSearchCount, 5, ExpiryDate, MachineID);
            }
        };
        
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
                std::string videoId = result.videoId;
                if (videoId.empty()) {
                    videoId = result.url;
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
        // Persist search count whenever the dialog increments it
        pSearchDialog->SetSearchCountCallback([this](int searchesUsed) {
            FreeSearchCount = searchesUsed;
            SaveLicense();
            LogInfo("Search count updated: " + std::to_string(FreeSearchCount) + "/5");
        });
        // Open License Manager when search requires license
        pSearchDialog->SetLicenseRequiredCallback([this]() {
            ShowLicenseDialog();
        });
    }
    ShowSearchWindow();
    // Push current license state to the dialog
    if (pSearchDialog) {
        pSearchDialog->SetLicenseStatus(IsLicensed, FreeSearchCount, 5, ExpiryDate, MachineID);
    }
}

void CSoundCloudSource::ShowSearchWindow()
{
    LogInfo("Showing search window...");
    if (SearchWindowThread.joinable()) {
        // Window already running, try to bring to front
        if (pSearchDialog && pSearchDialog->GetHWND()) {
            // Update license status before bringing to front
            pSearchDialog->SetLicenseStatus(IsLicensed, FreeSearchCount, 5, ExpiryDate, MachineID);
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
        HMODULE hModule = GetModuleHandleA("SoundCloudSource64.dll");
        if (!hModule) hModule = GetModuleHandleA("SoundCloudSource32.dll");
        hInst = (HINSTANCE)hModule;
#endif
        if (pSearchDialog) {
            pSearchDialog->Show(hInst);
            // Set initial license status
            pSearchDialog->SetLicenseStatus(IsLicensed, FreeSearchCount, 5, ExpiryDate, MachineID);
        }
    });
    // Don't detach - let the thread complete naturally
}

void CSoundCloudSource::StreamTrackFromSearch(const SearchResult& result, int deck)
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
    if (pSearchDialog) pSearchDialog->SetStatusText("Getting stream URL: " + result.title);

    // Use the full SoundCloud URL from search results
    std::string videoUrl = result.url;
    std::string videoId = result.videoId;
    if (videoId.empty()) {
        videoId = result.url;
    }
    // For yt-dlp, always use the full URL if available
    if (!result.url.empty()) {
        videoUrl = result.url;
    }
    LogInfo("StreamTrackFromSearch: videoId=" + videoId);
    LogInfo("StreamTrackFromSearch: videoUrl=" + videoUrl);

    // Use yt-dlp --get-url to obtain a direct streamable URL (~1-2 sec)
    std::string streamOutputFile = (std::filesystem::path(GlobalCacheDir) / ("stream_" + videoId + ".txt")).string();
    std::string streamBatFile    = (std::filesystem::path(GlobalCacheDir) / ("stream_" + videoId + ".bat")).string();

    // For video streaming, use the best single combined format available
    // bestvideo+bestaudio returns 2 separate URLs which VDJ can't handle for streaming
    // best[ext=mp4]/best returns a single muxed URL at the highest available quality
    std::string formatArg = (DownloadFormat == "mp4")
        ? "\"best[ext=mp4]/best\""
        : "\"bestaudio\"";

    {
        std::ofstream bat(streamBatFile);
        bat << "@echo off\n";
        bat << "\"" << ytDlpPath << "\" --no-warnings --get-url -f " << formatArg
            << " \"" << videoUrl << "\" > \"" << streamOutputFile << "\" 2>&1\n";
        bat.close();
    }

    std::vector<std::string> out;
    RunCommand("cmd /c \"" + streamBatFile + "\"", out);

    std::string directUrl;
    {
        std::ifstream sf(streamOutputFile);
        std::string line;
        while (std::getline(sf, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            if (!line.empty() && line.find("http") == 0) { directUrl = line; break; }
        }
    }
    try { std::filesystem::remove(streamBatFile); std::filesystem::remove(streamOutputFile); } catch (...) {}

    if (directUrl.empty()) {
        LogWarning("StreamTrackFromSearch: no direct URL, falling back to download");
        if (pSearchDialog) pSearchDialog->SetStatusText("Streaming failed, downloading instead...");
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
        if (pSearchDialog) pSearchDialog->SetStatusText("Stream failed, downloading instead...");
        LoadTrackFromSearch(result, deck);
        return;
    }

    if (pSearchDialog) pSearchDialog->SetStatusText("Streaming: " + result.title);
}

void CSoundCloudSource::LoadTrackFromSearch(const SearchResult& result, int deck)
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
    
    if (pSearchDialog) pSearchDialog->SetStatusText("Downloading: " + result.title);
    
    // Use the full URL from search results (the SoundCloud permalink)
    std::string videoId = result.url;
    if (videoId.empty()) {
        videoId = result.videoId;  // Fallback to numeric ID if URL is missing
    }
    
    LogInfo("Video ID: " + videoId);
    LogInfo("Original URL: " + result.url);
    
    std::string sanitizedTitle = SanitizeFilename(result.title);
    LogInfo("Sanitized title: " + sanitizedTitle);
    LogInfo("Download format: " + DownloadFormat);
    
    std::string cachedFile = DownloadAndCacheTrack(videoId, sanitizedTitle);
    
    if (cachedFile.empty()) {
        LogError("LoadTrackFromSearch: DownloadAndCacheTrack returned empty path for videoId=" + videoId);
        if (pSearchDialog) pSearchDialog->SetStatusText("Download failed!");
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
        if (pSearchDialog) pSearchDialog->SetStatusText("Error: file not found after download");
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
            if (pSearchDialog) pSearchDialog->SetStatusText("Load failed - see log for details");
            return;
        }
    }
    
    if (pSearchDialog) pSearchDialog->SetStatusText("Loaded: " + result.title);
}

//////////////////////////////////////////////////////////////////////////
// Command execution

bool CSoundCloudSource::RunCommand(const std::string& command, std::vector<std::string>& output)
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

void CSoundCloudSource::CleanupOldCache()
{
    LogInfo("Cleaning up cache files older than 12 hours...");
    try {
        if (GlobalCacheDir.empty() || !std::filesystem::exists(GlobalCacheDir)) return;

        auto now = std::chrono::system_clock::now();
        int count = 0;

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

void CSoundCloudSource::InitializeLogging()
{
    try {
        std::string moduleDir = GetModuleDirectory();
        LogFilePath = moduleDir + "\\SoundCloudSource.log";
        
        std::ofstream logFile(LogFilePath, std::ios::trunc);
        if (logFile.is_open()) {
            logFile.close();
            LogInfo("=== SoundCloud Online Source Plugin Log Started ===");
            LogInfo("DLL Location: " + moduleDir);
            LogInfo("Log file: " + LogFilePath);
        } else {
            std::filesystem::path tempLogPath = std::filesystem::temp_directory_path() / "YouTubeSource.log";
            LogFilePath = tempLogPath.string();
            
            std::ofstream fallbackLog(LogFilePath, std::ios::trunc);
            if (fallbackLog.is_open()) {
                fallbackLog.close();
                LogInfo("=== SoundCloud Online Source Plugin Log Started (Fallback) ===");
                LogInfo("DLL Location: " + moduleDir);
                LogInfo("Log file: " + LogFilePath);
            }
        }
    } catch (...) {}
}

void CSoundCloudSource::Log(LogLevel level, const std::string& message)
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

void CSoundCloudSource::LogDebug(const std::string& message) { Log(LOG_DEBUG, message); }
void CSoundCloudSource::LogInfo(const std::string& message) { Log(LOG_INFO, message); }
void CSoundCloudSource::LogWarning(const std::string& message) { Log(LOG_WARNING, message); }
void CSoundCloudSource::LogError(const std::string& message) { Log(LOG_ERROR, message); }

std::string CSoundCloudSource::GetTimestamp()
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

std::string CSoundCloudSource::GetLogLevelString(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string CSoundCloudSource::GetModuleDirectory()
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
    
    HMODULE hModule = GetModuleHandleA("SoundCloudSource64.dll");
    if (!hModule) hModule = GetModuleHandleA("SoundCloudSource32.dll");
    
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
    return std::filesystem::current_path().string();
#endif
}

std::string CSoundCloudSource::SanitizeFilename(const std::string& filename)
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

void CSoundCloudSource::LoadSettings()
{
    std::string settingsFile = GlobalCacheDir + "\\settings.ini";
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

void CSoundCloudSource::SaveSettings()
{
    std::string settingsFile = GlobalCacheDir + "\\settings.ini";
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

std::string CSoundCloudSource::EncryptPassword(const std::string& password)
{
    // Simple XOR encryption with machine ID as key
    std::string key = MachineID;
    if (key.empty()) key = "SoundCloudSource2024";
    
    std::string encrypted;
    for (size_t i = 0; i < password.length(); i++) {
        encrypted += (char)(password[i] ^ key[i % key.length()]);
    }
    
    // Convert to hex for safe storage
    std::string hex;
    for (unsigned char c : encrypted) {
        char buf[3];
        sprintf_s(buf, "%02X", c);
        hex += buf;
    }
    return hex;
}

std::string CSoundCloudSource::DecryptPassword(const std::string& encrypted)
{
    if (encrypted.empty()) return "";
    
    // Convert from hex
    std::string binary;
    for (size_t i = 0; i < encrypted.length(); i += 2) {
        std::string byteStr = encrypted.substr(i, 2);
        char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
        binary += byte;
    }
    
    // XOR decrypt with machine ID as key
    std::string key = MachineID;
    if (key.empty()) key = "SoundCloudSource2024";
    
    std::string decrypted;
    for (size_t i = 0; i < binary.length(); i++) {
        decrypted += (char)(binary[i] ^ key[i % key.length()]);
    }
    return decrypted;
}

void CSoundCloudSource::LoadLicense()
{
    std::string licenseFile = GlobalCacheDir + "\\license.dat";
    std::ifstream file(licenseFile);
    if (file.is_open()) {
        // Only load login status - no cached license data
        std::string loginStr, savePasswordStr;
        if (std::getline(file, loginStr)) {
            IsLoggedIn = (loginStr == "1");
        }
        std::getline(file, AccountEmail);
        std::getline(file, AccountName);
        std::getline(file, AuthToken);
        std::getline(file, SavedPassword);  // Load encrypted password
        if (std::getline(file, savePasswordStr)) {
            SavePasswordEnabled = (savePasswordStr == "1");
        }
        file.close();
        LogInfo("Loaded login status: logged_in=" + std::string(IsLoggedIn ? "yes" : "no") + " email=" + AccountEmail);
    }
}

void CSoundCloudSource::SaveLicense()
{
    std::string licenseFile = GlobalCacheDir + "\\license.dat";
    std::ofstream file(licenseFile);
    if (file.is_open()) {
        // Only save login status - no cached license data
        file << (IsLoggedIn ? "1" : "0") << std::endl;  // Marketplace login state
        file << AccountEmail << std::endl;
        file << AccountName << std::endl;
        file << AuthToken << std::endl;
        file << SavedPassword << std::endl;  // Save encrypted password
        file << (SavePasswordEnabled ? "1" : "0") << std::endl;  // Save password preference
        file.close();
    }
}

void CSoundCloudSource::UpdateLicenseStatus(bool licensed)
{
    if (IsLicensed != licensed) {
        IsLicensed = licensed;
        LogInfo("License status changed: " + std::string(licensed ? "LICENSED" : "UNLICENSED"));
        
        // Update SearchDialog via Windows message for reliable cross-thread UI update
        if (pSearchDialog) {
            HWND hwnd = pSearchDialog->GetHWND();
            if (hwnd && IsWindow(hwnd)) {
                // First update the license data
                pSearchDialog->SetLicenseStatus(licensed, FreeSearchCount, 5, ExpiryDate, MachineID);
                // Then post message to refresh UI on main thread
                PostMessage(hwnd, WM_LICENSE_STATUS_CHANGED, 0, 0);
                LogInfo("Posted WM_LICENSE_STATUS_CHANGED message to SearchDialog");
            }
        }
        
        // Update LicenseDialogV2 if it's open
        if (hLicenseDialogWindow && IsWindow(hLicenseDialogWindow)) {
            // Post message to refresh license dialog UI
            PostMessage(hLicenseDialogWindow, WM_LICENSE_STATUS_CHANGED, 0, 0);
            LogInfo("Posted WM_LICENSE_STATUS_CHANGED message to LicenseDialogV2");
        }
        
        // Also call callback if set (for backward compatibility)
        if (OnLicenseStatusChanged) {
            OnLicenseStatusChanged(licensed);
        }
    }
}

void CSoundCloudSource::CheckLicenseBackground()
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

void CSoundCloudSource::StartLicenseTimer()
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

void CSoundCloudSource::StopLicenseTimer()
{
    StopTimer = true;
    
    if (LicenseTimerThread.joinable()) {
        LicenseTimerThread.join();
    }
}

bool CSoundCloudSource::CheckLicense()
{
    LoadLicense();
    
    // Check if user is logged into marketplace account
    if (!IsLoggedIn || AccountEmail.empty() || AuthToken.empty()) {
        LogInfo("No valid marketplace account login found");
        // Clear any cached license data
        LicenseKey = "";
        ExpiryDate = "";
        DeviceCount = 0;
        MaxDevices = 0;
        return false;
    }
    
    LogInfo("User logged into marketplace account: " + AccountEmail);
    
    // ALWAYS validate with server - no offline mode, no cached data
    if (LicenseServerURL.empty()) {
        LogWarning("No license server URL configured");
        return false;
    }
    
    LogInfo("Validating license with server (real-time check)...");
    
    // Store current login state before validation
    bool wasLoggedIn = IsLoggedIn;
    std::string savedAuthToken = AuthToken;
    
    bool serverValid = ValidateLicenseOnline();
    
    if (!serverValid) {
        // Check if we have valid login session with license key - keep session active
        // (ExpiryDate can be null/empty for lifetime licenses)
        if (!LicenseKey.empty() && !AuthToken.empty()) {
            LogInfo("Server validation failed but we have active login session with license key - keeping session active");
            return true;
        }
        
        LogWarning("Server validation failed and no valid session data - clearing login");
        // Clear login data and all license info
        IsLoggedIn = false;
        AuthToken = "";
        LicenseKey = "";
        ExpiryDate = "";
        DeviceCount = 0;
        MaxDevices = 0;
        SaveLicense();
        return false;
    }
    
    LogInfo("Server validation successful - license is active");
    LogInfo("License data from server: key=" + LicenseKey + " expiry=" + ExpiryDate + " devices=" + std::to_string(DeviceCount) + "/" + std::to_string(MaxDevices));
    return true;
}

// Dialog procedure for the activation input box
static HWND g_hKeyEdit = NULL;
static std::string g_activationResult;

static LRESULT CALLBACK ActivationDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        // Label
        CreateWindowA("STATIC", "Enter your license key (format: SC-XXXX-XXXX-XXXX-XXXX-XXXX):",
            WS_CHILD | WS_VISIBLE, 12, 12, 360, 20, hwnd, NULL, NULL, NULL);
        // Edit control
        g_hKeyEdit = CreateWindowA("EDIT", "SC-",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_UPPERCASE | ES_AUTOHSCROLL,
            12, 38, 360, 24, hwnd, (HMENU)101, NULL, NULL);
        // Activate button
        CreateWindowA("BUTTON", "Activate",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            12, 74, 100, 28, hwnd, (HMENU)IDOK, NULL, NULL);
        // Cancel button
        CreateWindowA("BUTTON", "Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            120, 74, 80, 28, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        SetFocus(g_hKeyEdit);
        SendMessageA(g_hKeyEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            char buf[64] = {};
            GetWindowTextA(g_hKeyEdit, buf, sizeof(buf));
            g_activationResult = buf;
            DestroyWindow(hwnd);
        } else if (LOWORD(wp) == IDCANCEL) {
            g_activationResult = "";
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

void CSoundCloudSource::ShowActivationDialog()
{
    g_activationResult = "";

    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc   = ActivationDlgProc;
    wc.hInstance     = g_hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "SCActivationDlg";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc); // ignore if already registered

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "SCActivationDlg", "SoundCloud Source - Activate License",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 148,
        NULL, NULL, g_hInstance, NULL);

    if (!hwnd) {
        // Fallback: tell user to type in search box
        MessageBoxA(NULL,
            "To activate, type in the VirtualDJ search box:\n\nACTIVATE:SC-XXXX-XXXX-XXXX",
            "SoundCloud Source - Activate License", MB_OK | MB_ICONINFORMATION);
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Modal message loop
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // Process result
    if (!g_activationResult.empty()) {
        if (ActivateLicense(g_activationResult)) {
            MessageBoxA(NULL,
                ("License activated!\nValid until: " + ExpiryDate + "\n\nSoundCloud Source is now fully unlocked.").c_str(),
                "Activation Successful", MB_OK | MB_ICONINFORMATION);
            LogActivation(g_activationResult, true);
        } else {
            std::string errorMsg = "Invalid or mismatched license key.\n\n"
                                  "Keys must be bound to this specific machine.\n"
                                  "Your Machine ID: " + MachineID + "\n\n"
                                  "Please contact support with your Machine ID "
                                  "to get a key bound to your computer.";
            MessageBoxA(NULL, errorMsg.c_str(), "Activation Failed", MB_OK | MB_ICONERROR);
            LogActivation(g_activationResult, false);
        }
    }
}

void CSoundCloudSource::ShowLicenseDialog()
{
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
        IsLicensed = (!LicenseKey.empty() && !AuthToken.empty());

        LicenseDialogV2 dlg;
        dlg.SetLicenseInfo(LicenseKey, ExpiryDate, IsLicensed);
        dlg.SetMachineID(MachineID);
        dlg.SetMarketplaceAccount(IsLoggedIn, AccountEmail, DeviceCount, MaxDevices);
        dlg.SetTrialInfo(FreeSearchCount, 5);
        dlg.SetSavedEmail(AccountEmail);  // Pre-fill email field with saved email
        dlg.SetSavedPassword(DecryptPassword(SavedPassword));  // Pre-fill password field with decrypted password
        dlg.SetSavePasswordEnabled(SavePasswordEnabled);  // Set checkbox state

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
}

bool CSoundCloudSource::ActivateLicense(const std::string& key)
{
    // Try online activation first (new format: SC-XXXX-XXXX-XXXX-XXXX-XXXX)
    if (!LicenseServerURL.empty()) {
        std::regex onlinePattern(R"(SC-\d{4}-\d{4}-\d{4}-\d{4}-\d{4})");
        if (std::regex_match(key, onlinePattern)) {
            LogInfo("Attempting online activation for key: " + key);
            if (ActivateLicenseOnline(key)) {
                return true;
            }
            // If server returned a definitive rejection, fail
            // If it was a connection error, fall through to offline
            LogWarning("Online activation failed, trying offline fallback...");
        }
    }
    
    // No offline activation - all licenses must be validated online
    LogWarning("Offline activation not supported - please use marketplace login");
    return false;
}

void CSoundCloudSource::LogActivation(const std::string& key, bool success)
{
    // Get current timestamp
    std::time_t now = std::time(nullptr);
    std::tm tm = {};
    localtime_s(&tm, &now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    // Log to activations file in cache directory
    std::string logFile = GlobalCacheDir + "\\activations.log";
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
std::string CSoundCloudSource::GetComputerName()
{
    char buf[256] = {};
    DWORD size = sizeof(buf);
    if (GetComputerNameA(buf, &size)) {
        return std::string(buf);
    }
    return "UNKNOWN";
}

// Generate hardware fingerprint
std::string CSoundCloudSource::GetMachineID()
{
    // Get multiple hardware identifiers for a unique fingerprint
    std::string fingerprint;
    
    // 1. Computer name
    char computerName[256] = {};
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        fingerprint += std::string(computerName);
    }
    
    // 2. Volume serial number of system drive
    char systemDir[MAX_PATH] = {};
    GetWindowsDirectoryA(systemDir, sizeof(systemDir));
    systemDir[3] = '\0'; // Extract drive letter (e.g., "C:\")
    
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

std::string CSoundCloudSource::HttpPost(const std::string& url, const std::string& postData)
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

bool CSoundCloudSource::ValidateLicenseOnline()
{
    if (LicenseServerURL.empty() || LicenseKey.empty()) return false;
    
    std::string url = LicenseServerURL + "/pages/api/validate.php";
    std::string postData = "key=" + LicenseKey + "&machine_id=" + MachineID;
    
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

bool CSoundCloudSource::ActivateLicenseOnline(const std::string& key)
{
    if (LicenseServerURL.empty()) {
        LogWarning("No license server URL configured");
        return false;
    }
    
    // Validate key format: SC-XXXX-XXXX-XXXX-XXXX-XXXX
    std::regex keyPattern(R"(SC-\d{4}-\d{4}-\d{4}-\d{4}-\d{4})");
    if (!std::regex_match(key, keyPattern)) {
        LogWarning("Invalid key format for online activation: " + key);
        return false;
    }
    
    std::string url = LicenseServerURL + "/pages/api/activate.php";
    std::string computerName = GetComputerName();
    std::string postData = "key=" + key + "&machine_id=" + MachineID + "&computer_name=" + computerName;
    
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
        ExpiryDate = expiry.substr(0, 10);  // YYYY-MM-DD
        IsLicensed = true;
        SaveLicense();
        LogInfo("Online activation SUCCESS: " + key + " expires " + ExpiryDate);
        return true;
    }
    
    LogWarning("Online activation FAILED: " + status + " - " + message);
    return false;
}

//////////////////////////////////////////////////////////////////////////
// Marketplace account integration

bool CSoundCloudSource::LoginToMarketplace(const std::string& email, const std::string& password, bool savePassword)
{
    if (LicenseServerURL.empty()) {
        LogWarning("No license server URL configured");
        return false;
    }
    
    std::string url = LicenseServerURL + "/pages/api/login";
    std::string postData = "email=" + email + "&password=" + password + "&plugin=soundcloud-source&device_name=" + GetComputerName();
    
    LogInfo("Attempting marketplace login for: " + email);
    std::string response = HttpPost(url, postData);
    
    if (response.empty()) {
        LogError("Login failed (no response from server)");
        return false;
    }
    
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
        std::string numStr = json.substr(pos, end - pos);
        try {
            return std::stoi(numStr);
        } catch (...) {
            return 0;
        }
    };
    
    std::string status = getJsonValue(response, "status");
    std::string message = getJsonValue(response, "message");
    
    if (status == "active") {
        // Login successful
        IsLoggedIn = true;
        AccountEmail = email;
        AccountName = email.substr(0, email.find('@'));
        AuthToken = getJsonValue(response, "auth_token");
        LicenseKey = getJsonValue(response, "license_key");
        ExpiryDate = getJsonValue(response, "expires");
        DeviceCount = getJsonInt(response, "current_activations");
        MaxDevices = getJsonInt(response, "max_activations");
        
        // Save encrypted password only if user wants to
        SavePasswordEnabled = savePassword;
        if (savePassword) {
            SavedPassword = EncryptPassword(password);
            LogInfo("Password will be saved (encrypted)");
        } else {
            SavedPassword = "";
            LogInfo("Password will NOT be saved");
        }
        
        if (!LicenseKey.empty()) {
            IsLicensed = true;
        }
        
        SaveLicense();
        LogInfo("Marketplace login SUCCESS for: " + email);
        return true;
    }
    
    // Handle error status - just log it, UI will handle display
    if (status == "error") {
        LogWarning("Marketplace login FAILED: " + status + " - " + message);
        return false;
    }
    
    LogWarning("Marketplace login FAILED: " + status);
    return false;
}

void CSoundCloudSource::LogoutFromMarketplace()
{
    // Keep email saved for convenience, only clear login session
    IsLoggedIn = false;
    AccountName = "";
    AuthToken = "";
    DeviceCount = 0;
    MaxDevices = 0;
    
    SaveLicense();
    LogInfo("Logged out from marketplace (email saved for next login)");
}

void CSoundCloudSource::OpenMarketplaceDashboard()
{
    std::string url = LicenseServerURL + "/dashboard";
    LogInfo("Opening marketplace dashboard: " + url);
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

//////////////////////////////////////////////////////////////////////////
// Auto-update system

void CSoundCloudSource::CheckForUpdates()
{
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

bool CSoundCloudSource::HttpDownloadFile(const std::string& url, const std::string& localPath)
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

void CSoundCloudSource::PerformUpdate()
{
#ifdef _WIN64
    std::string dlUrl  = UpdateDownloadUrl64;
    std::string dll    = "SoundCloudSource64.dll";
#else
    std::string dlUrl  = UpdateDownloadUrl32;
    std::string dll    = "SoundCloudSource32.dll";
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

//////////////////////////////////////////////////////////////////////////
// DLL entry point

extern "C" VDJ_EXPORT HRESULT VDJ_API DllGetClassObject(const GUID &rclsid, const GUID &riid, void** ppObject)
{
    if (rclsid == CLSID_VdjPlugin8) {
        if (riid == IID_IVdjPluginOnlineSource) {
            *ppObject = new CSoundCloudSource();
            return S_OK;
        }
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}
