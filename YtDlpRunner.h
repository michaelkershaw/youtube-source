#ifndef YTDLPRUNNER_H
#define YTDLPRUNNER_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <string>
#include <vector>
#include <functional>
#include <atomic>

// Lightweight video metadata parsed from yt-dlp JSON output
struct VideoInfo {
    std::string id;
    std::string title;
    std::string channel;
    std::string durationStr;   // "3:45"
    std::string url;           // webpage url
    long long   views = 0;
    double      durationSec = 0;
};

// Single choke-point for all yt-dlp process invocations.
// Windows: CreateProcess with piped stdout. macOS/Linux: fork/exec with pipes.
// (no .bat files, no temp txt files)
class YtDlpRunner {
public:
    static const unsigned long kNoTimeout = 0xFFFFFFFFul;
    struct Result {
        int exitCode = -1;
        bool timedOut = false;
        bool cancelled = false;
        bool started = false;
        std::vector<std::string> lines;   // merged stdout+stderr, one entry per line
        bool ok() const { return started && exitCode == 0 && !timedOut && !cancelled; }
    };

    using LineCallback = std::function<void(const std::string& line)>;
    using Logger = std::function<void(const std::string& message)>;

    YtDlpRunner() = default;

    void SetExePath(const std::string& exePath) { ExePath = exePath; }
    const std::string& GetExePath() const { return ExePath; }
    void SetLogger(Logger logger) { Log = std::move(logger); }

    // Run yt-dlp with the given arguments (already individually unquoted).
    // onLine (optional) is invoked live for every output line (progress parsing).
    Result Run(const std::vector<std::string>& args,
               unsigned long timeoutMs = 60000,
               std::atomic<bool>* cancelFlag = nullptr,
               LineCallback onLine = nullptr) const;

    // Run with automatic retry (network hiccups). No retry on cancel.
    Result RunWithRetry(const std::vector<std::string>& args,
                        int retries = 2,
                        unsigned long timeoutMs = 60000,
                        std::atomic<bool>* cancelFlag = nullptr,
                        LineCallback onLine = nullptr) const;

    // Quote a single argument for a command line (Windows rules; unused on POSIX).
    static std::string QuoteArg(const std::string& arg);

    // Parse one line of `--print "%(.{id,title,uploader,duration_string,view_count,webpage_url})j"` output.
    // Returns false if the line is not valid JSON video info.
    static bool ParseVideoJsonLine(const std::string& line, VideoInfo& out);

private:
    std::string ExePath;
    Logger Log;
    void LogMsg(const std::string& msg) const { if (Log) Log(msg); }
};

#endif
