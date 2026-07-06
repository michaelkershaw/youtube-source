#include "YtDlpRunner.h"
#include "json.hpp"
#include <thread>
#include <mutex>
#include <sstream>
#include <chrono>
#include <cstdio>

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#endif

using json = nlohmann::json;

std::string YtDlpRunner::QuoteArg(const std::string& arg)
{
    if (arg.empty()) return "\"\"";
    if (arg.find_first_of(" \t\"") == std::string::npos) return arg;

    std::string out = "\"";
    size_t backslashes = 0;
    for (char c : arg) {
        if (c == '\\') {
            backslashes++;
        } else if (c == '"') {
            out.append(backslashes * 2 + 1, '\\');
            out.push_back('"');
            backslashes = 0;
        } else {
            out.append(backslashes, '\\');
            out.push_back(c);
            backslashes = 0;
        }
    }
    out.append(backslashes * 2, '\\');
    out.push_back('"');
    return out;
}

#ifdef _WIN32

YtDlpRunner::Result YtDlpRunner::Run(const std::vector<std::string>& args,
                                     unsigned long timeoutMs,
                                     std::atomic<bool>* cancelFlag,
                                     LineCallback onLine) const
{
    Result result;

    // Build command line
    std::string cmdLine = QuoteArg(ExePath);
    for (const auto& a : args) {
        cmdLine += " ";
        cmdLine += QuoteArg(a);
    }
    LogMsg("YtDlpRunner: " + cmdLine);

    // Create pipe for stdout/stderr
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        LogMsg("YtDlpRunner: CreatePipe failed");
        return result;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = NULL;

    PROCESS_INFORMATION pi = {};
    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    if (!CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        LogMsg("YtDlpRunner: CreateProcess failed, err=" + std::to_string(GetLastError()));
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return result;
    }
    result.started = true;
    CloseHandle(hWritePipe);  // child owns write end now

    // Reader thread: blocking ReadFile loop. Unblocks when process exits (pipe EOF)
    // or when we terminate the process on cancel/timeout.
    std::mutex linesMtx;
    std::thread reader([&]() {
        std::string pending;
        char buf[4096];
        DWORD bytesRead = 0;
        while (ReadFile(hReadPipe, buf, sizeof(buf), &bytesRead, NULL) && bytesRead > 0) {
            pending.append(buf, bytesRead);
            size_t pos;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
                if (line.empty()) continue;
                {
                    std::lock_guard<std::mutex> lk(linesMtx);
                    result.lines.push_back(line);
                }
                if (onLine) { try { onLine(line); } catch (...) {} }
            }
        }
        // flush trailing partial line
        while (!pending.empty() && (pending.back() == '\r' || pending.back() == '\n')) pending.pop_back();
        if (!pending.empty()) {
            std::lock_guard<std::mutex> lk(linesMtx);
            result.lines.push_back(pending);
            if (onLine) { try { onLine(pending); } catch (...) {} }
        }
    });

    // Wait for exit, polling for cancellation
    const DWORD sliceMs = 200;
    DWORD waited = 0;
    for (;;) {
        DWORD w = WaitForSingleObject(pi.hProcess, sliceMs);
        if (w == WAIT_OBJECT_0) break;
        waited += sliceMs;
        if (cancelFlag && cancelFlag->load()) {
            LogMsg("YtDlpRunner: cancelled, terminating process");
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 3000);
            result.cancelled = true;
            break;
        }
        if (timeoutMs != INFINITE && waited >= timeoutMs) {
            LogMsg("YtDlpRunner: timeout after " + std::to_string(waited) + "ms, terminating process");
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 3000);
            result.timedOut = true;
            break;
        }
    }

    DWORD exitCode = (DWORD)-1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = (int)exitCode;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (reader.joinable()) reader.join();
    CloseHandle(hReadPipe);

    return result;
}

#else  // POSIX (macOS / Linux)

YtDlpRunner::Result YtDlpRunner::Run(const std::vector<std::string>& args,
                                     unsigned long timeoutMs,
                                     std::atomic<bool>* cancelFlag,
                                     LineCallback onLine) const
{
    Result result;

    std::string logLine = ExePath;
    for (const auto& a : args) { logLine += " "; logLine += a; }
    LogMsg("YtDlpRunner: " + logLine);

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        LogMsg("YtDlpRunner: pipe() failed");
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        LogMsg("YtDlpRunner: fork() failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return result;
    }

    if (pid == 0) {
        // Child: redirect stdout+stderr to the pipe, exec yt-dlp
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(ExePath.c_str()));
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execv(ExePath.c_str(), argv.data());
        _exit(127);  // exec failed
    }

    // Parent
    result.started = true;
    close(pipefd[1]);

    std::mutex linesMtx;
    std::thread reader([&]() {
        std::string pending;
        char buf[4096];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
            pending.append(buf, (size_t)n);
            size_t pos;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
                if (line.empty()) continue;
                {
                    std::lock_guard<std::mutex> lk(linesMtx);
                    result.lines.push_back(line);
                }
                if (onLine) { try { onLine(line); } catch (...) {} }
            }
        }
        while (!pending.empty() && (pending.back() == '\r' || pending.back() == '\n')) pending.pop_back();
        if (!pending.empty()) {
            std::lock_guard<std::mutex> lk(linesMtx);
            result.lines.push_back(pending);
            if (onLine) { try { onLine(pending); } catch (...) {} }
        }
    });

    // Wait for exit, polling for cancellation / timeout
    const unsigned long sliceMs = 200;
    unsigned long waited = 0;
    int status = 0;
    for (;;) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) break;
        if (w < 0) { status = -1; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(sliceMs));
        waited += sliceMs;
        if (cancelFlag && cancelFlag->load()) {
            LogMsg("YtDlpRunner: cancelled, terminating process");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.cancelled = true;
            break;
        }
        if (timeoutMs != kNoTimeout && waited >= timeoutMs) {
            LogMsg("YtDlpRunner: timeout after " + std::to_string(waited) + "ms, terminating process");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.timedOut = true;
            break;
        }
    }

    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if (reader.joinable()) reader.join();
    close(pipefd[0]);

    return result;
}

#endif  // _WIN32

YtDlpRunner::Result YtDlpRunner::RunWithRetry(const std::vector<std::string>& args,
                                              int retries,
                                              unsigned long timeoutMs,
                                              std::atomic<bool>* cancelFlag,
                                              LineCallback onLine) const
{
    Result r;
    int attempt = 0;
    unsigned long backoffMs = 750;
    for (;;) {
        r = Run(args, timeoutMs, cancelFlag, onLine);
        if (r.ok() || r.cancelled) return r;
        if (attempt >= retries) return r;
        attempt++;
        LogMsg("YtDlpRunner: retry " + std::to_string(attempt) + "/" + std::to_string(retries));
        unsigned long slept = 0;
        while (slept < backoffMs) {
            if (cancelFlag && cancelFlag->load()) { r.cancelled = true; return r; }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            slept += 100;
        }
        backoffMs *= 2;
    }
}

bool YtDlpRunner::ParseVideoJsonLine(const std::string& line, VideoInfo& out)
{
    if (line.empty() || line[0] != '{') return false;
    try {
        json j = json::parse(line);
        if (!j.contains("id") || j["id"].is_null()) return false;

        out.id = j["id"].get<std::string>();
        out.title = (j.contains("title") && j["title"].is_string()) ? j["title"].get<std::string>() : "";
        if (j.contains("uploader") && j["uploader"].is_string()) out.channel = j["uploader"].get<std::string>();
        else if (j.contains("channel") && j["channel"].is_string()) out.channel = j["channel"].get<std::string>();

        if (j.contains("duration_string") && j["duration_string"].is_string())
            out.durationStr = j["duration_string"].get<std::string>();

        if (j.contains("duration") && j["duration"].is_number())
            out.durationSec = j["duration"].get<double>();

        if (out.durationSec == 0 && !out.durationStr.empty()) {
            // Parse "1:02:30" / "3:45" / "45"
            std::istringstream dss(out.durationStr);
            std::string part;
            std::vector<int> parts;
            while (std::getline(dss, part, ':')) {
                try { parts.push_back(std::stoi(part)); } catch (...) {}
            }
            if (parts.size() == 3) out.durationSec = parts[0] * 3600.0 + parts[1] * 60.0 + parts[2];
            else if (parts.size() == 2) out.durationSec = parts[0] * 60.0 + parts[1];
            else if (parts.size() == 1) out.durationSec = parts[0];
        }
        if (out.durationStr.empty() && out.durationSec > 0) {
            int total = (int)out.durationSec;
            char buf[32];
            if (total >= 3600) snprintf(buf, sizeof(buf), "%d:%02d:%02d", total / 3600, (total % 3600) / 60, total % 60);
            else snprintf(buf, sizeof(buf), "%d:%02d", total / 60, total % 60);
            out.durationStr = buf;
        }

        if (j.contains("view_count") && j["view_count"].is_number())
            out.views = j["view_count"].get<long long>();

        if (j.contains("webpage_url") && j["webpage_url"].is_string())
            out.url = j["webpage_url"].get<std::string>();
        else
            out.url = "https://www.youtube.com/watch?v=" + out.id;

        return !out.title.empty();
    } catch (...) {
        return false;
    }
}
