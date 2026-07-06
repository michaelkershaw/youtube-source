#ifndef CACHEINDEX_H
#define CACHEINDEX_H

#include <string>
#include <vector>
#include <mutex>

// Persistent JSON index of cached downloads (videoId -> file metadata).
// Replaces per-search filesystem scans and the fixed 12h cleanup sweep.
class CacheIndex {
public:
    struct Entry {
        std::string videoId;
        std::string title;
        std::string channel;
        std::string duration;   // "3:45"
        std::string format;     // "mp3" or "mp4"
        std::string path;       // absolute file path
        long long   addedAt = 0; // unix seconds
    };

    // Load index from <cacheDir>/cache_index.json, then reconcile with files on disk.
    void Load(const std::string& cacheDir);
    void Save() const;

    void Add(const Entry& entry);
    bool Has(const std::string& videoId, const std::string& format) const;
    std::string PathFor(const std::string& videoId, const std::string& format) const;
    std::vector<Entry> All() const;

    // Remove entries whose file no longer exists; adopt untracked mp3/mp4 files found in cacheDir.
    void Reconcile();

    // Delete files older than ttlSeconds (0 = keep forever). Returns number removed.
    int Cleanup(long long ttlSeconds);

private:
    mutable std::mutex Mtx;
    std::string CacheDir;
    std::string IndexPath;
    std::vector<Entry> Entries;

    void SaveLocked() const;  // requires Mtx held
};

#endif
