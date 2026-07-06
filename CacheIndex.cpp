#include "CacheIndex.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>

using json = nlohmann::json;
namespace fs = std::filesystem;

void CacheIndex::Load(const std::string& cacheDir)
{
    std::lock_guard<std::mutex> lk(Mtx);
    CacheDir = cacheDir;
    IndexPath = (fs::path(cacheDir) / "cache_index.json").string();
    Entries.clear();

    try {
        std::ifstream f(IndexPath);
        if (f.good()) {
            json j = json::parse(f, nullptr, false);
            if (j.is_array()) {
                for (const auto& item : j) {
                    Entry e;
                    e.videoId  = item.value("videoId", "");
                    e.title    = item.value("title", "");
                    e.channel  = item.value("channel", "");
                    e.duration = item.value("duration", "");
                    e.format   = item.value("format", "");
                    e.path     = item.value("path", "");
                    e.addedAt  = item.value("addedAt", 0LL);
                    if (!e.videoId.empty() && !e.path.empty())
                        Entries.push_back(e);
                }
            }
        }
    } catch (...) {}
}

void CacheIndex::SaveLocked() const
{
    try {
        json j = json::array();
        for (const auto& e : Entries) {
            j.push_back({
                {"videoId",  e.videoId},
                {"title",    e.title},
                {"channel",  e.channel},
                {"duration", e.duration},
                {"format",   e.format},
                {"path",     e.path},
                {"addedAt",  e.addedAt}
            });
        }
        std::ofstream f(IndexPath, std::ios::trunc);
        f << j.dump(1);
    } catch (...) {}
}

void CacheIndex::Save() const
{
    std::lock_guard<std::mutex> lk(Mtx);
    SaveLocked();
}

void CacheIndex::Add(const Entry& entry)
{
    std::lock_guard<std::mutex> lk(Mtx);
    for (auto& e : Entries) {
        if (e.videoId == entry.videoId && e.format == entry.format) {
            e = entry;
            SaveLocked();
            return;
        }
    }
    Entries.push_back(entry);
    SaveLocked();
}

bool CacheIndex::Has(const std::string& videoId, const std::string& format) const
{
    std::lock_guard<std::mutex> lk(Mtx);
    for (const auto& e : Entries)
        if (e.videoId == videoId && e.format == format) return true;
    return false;
}

std::string CacheIndex::PathFor(const std::string& videoId, const std::string& format) const
{
    std::lock_guard<std::mutex> lk(Mtx);
    for (const auto& e : Entries)
        if (e.videoId == videoId && e.format == format) return e.path;
    return "";
}

std::vector<CacheIndex::Entry> CacheIndex::All() const
{
    std::lock_guard<std::mutex> lk(Mtx);
    return Entries;
}

void CacheIndex::Reconcile()
{
    std::lock_guard<std::mutex> lk(Mtx);
    bool changed = false;

    // Drop entries whose file no longer exists
    for (auto it = Entries.begin(); it != Entries.end();) {
        std::error_code ec;
        if (!fs::exists(it->path, ec)) {
            it = Entries.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    // Adopt untracked mp3/mp4 files named <videoId>.<ext>
    try {
        for (const auto& f : fs::directory_iterator(CacheDir)) {
            if (!f.is_regular_file()) continue;
            std::string ext = f.path().extension().string();
            if (ext != ".mp3" && ext != ".mp4") continue;
            std::string id = f.path().stem().string();
            std::string format = ext.substr(1);
            bool known = false;
            for (const auto& e : Entries)
                if (e.videoId == id && e.format == format) { known = true; break; }
            if (!known) {
                Entry e;
                e.videoId = id;
                e.title = id;
                e.format = format;
                e.path = f.path().string();
                auto ftime = fs::last_write_time(f.path());
                auto sys = std::chrono::time_point_cast<std::chrono::seconds>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                e.addedAt = (long long)std::chrono::duration_cast<std::chrono::seconds>(
                    sys.time_since_epoch()).count();
                Entries.push_back(e);
                changed = true;
            }
        }
    } catch (...) {}

    if (changed) SaveLocked();
}

int CacheIndex::Cleanup(long long ttlSeconds)
{
    if (ttlSeconds <= 0) return 0;
    std::lock_guard<std::mutex> lk(Mtx);
    long long now = (long long)std::time(nullptr);
    int removed = 0;

    for (auto it = Entries.begin(); it != Entries.end();) {
        if (now - it->addedAt > ttlSeconds) {
            std::error_code ec;
            fs::remove(it->path, ec);
            it = Entries.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    if (removed > 0) SaveLocked();
    return removed;
}
