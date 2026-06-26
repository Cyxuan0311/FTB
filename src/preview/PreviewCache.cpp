#include "preview/PreviewCache.hpp"

#include <chrono>
#include <thread>
#include <algorithm>

#include "config/ConfigManager.hpp"
#include "browser/SortMode.hpp"
#include "utils/PerfLogger.hpp"

namespace FTB {

PreviewCache& PreviewCache::Instance() {
    static PreviewCache instance;
    return instance;
}

PreviewData PreviewCache::Copy() {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_;
}

void PreviewCache::Invalidate() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_ = PreviewData{};
}

void PreviewCache::Update(const std::vector<FileManager::DirEntryInfo>& entries,
                          int selected, const std::string& currentPath) {
    PERF_SCOPE("cache");
    std::string new_key = currentPath + ":" + std::to_string(selected);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.key == new_key) {
            PERF_LOG("cache", "PreviewCache::Update: cache hit for " + new_key);
            auto now = std::chrono::steady_clock::now();
            auto since_last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_update_time_).count();
            if (since_last_update > 0 && since_last_update >= 50)
                preview_pending_ = true;
            return;
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time_).count();
    last_update_time_ = now;
    bool navigating_rapidly = (elapsed > 0 && elapsed < 50);

    PreviewData new_data;
    new_data.key = new_key;

    if (selected < 0 || selected >= static_cast<int>(entries.size())) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = std::move(new_data);
        preview_pending_ = false;
        return;
    }

    const FileManager::DirEntryInfo& entry = entries[selected];
    new_data.selectedName = entry.name;
    new_data.is_dir = entry.is_dir;
    new_data.exists = entry.exists;
    new_data.is_symlink = entry.is_symlink;
    new_data.is_executable = entry.is_executable;
    new_data.is_regular = entry.is_regular;
    new_data.file_size = entry.file_size;
    new_data.mod_time = entry.mod_time;
    new_data.icon = entry.icon;

    preview_pending_ = !navigating_rapidly;

    std::lock_guard<std::mutex> lock(mutex_);
    data_ = std::move(new_data);
}

void PreviewCache::EnsureDirLoaded(const std::string& dirPath) {
    PERF_SCOPE("cache");
    if (!preview_pending_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.dir_loaded) return;
    data_.dir_loaded = true;
    data_.loaded_dir_path = dirPath;
    PERF_LOG("cache", "EnsureDirLoaded: " + dirPath);

    std::thread([this, dirPath]() {
        PERF_LOG("cache", "EnsureDirLoaded thread start: " + dirPath);
        try {
            auto entries = FileManager::getDirectoryEntries(dirPath);
            {
                auto& cfg = FTB::ConfigManager::GetInstance()->GetConfig();
                auto mode = FTB::SortModeFromString(cfg.style.sort_mode);
                FTB::SortEntries(entries, mode);
            }
            std::lock_guard<std::mutex> lock2(mutex_);
            if (data_.loaded_dir_path != dirPath) return;
            data_.dir_contents = std::move(entries);
            data_.dir_sorted = true;
            PERF_LOG("cache", "EnsureDirLoaded thread done: " + dirPath);
        } catch (...) {
            std::lock_guard<std::mutex> lock2(mutex_);
            if (data_.loaded_dir_path == dirPath) data_.dir_contents.clear();
        }
    }).detach();
}

void PreviewCache::EnsureTextLoaded(const std::string& filePath, uintmax_t fileSize) {
    PERF_SCOPE("cache");
    if (!preview_pending_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.text_loaded) return;
    data_.text_loaded = true;
    data_.loaded_text_path = filePath;
    PERF_LOG("cache", "EnsureTextLoaded: " + filePath);

    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    int max_file_size_kb = cfg.preview.max_text_file_size_kb;
    int max_lines = cfg.preview.max_text_lines;
    int chunk_size = cfg.preview.chunk_size_lines;

    if (max_file_size_kb > 0 && fileSize > static_cast<uintmax_t>(max_file_size_kb) * 1024) {
        PERF_LOG("cache", "EnsureTextLoaded skipped (file too large): " + filePath);
        return;
    }

    int end_line = (max_lines > 0) ? max_lines : (chunk_size > 0 ? chunk_size : 100000);

    std::thread([this, filePath, end_line]() {
        PERF_LOG("cache", "EnsureTextLoaded thread start: " + filePath);
        try {
            auto content = FileManager::readFileContent(filePath, 1, end_line);
            std::lock_guard<std::mutex> lock2(mutex_);
            if (data_.loaded_text_path != filePath) return;
            data_.text_preview = std::move(content);
            data_.text_total_lines = end_line;
            PERF_LOG("cache", "EnsureTextLoaded thread done: " + filePath);
        } catch (...) {
            std::lock_guard<std::mutex> lock2(mutex_);
            if (data_.loaded_text_path == filePath) data_.text_preview.clear();
        }
    }).detach();
}

void PreviewCache::LoadMoreTextLines(const std::string& filePath, int from_line, int count) {
    if (!preview_pending_) {
        return;
    }
    PERF_LOG("cache", "LoadMoreTextLines: " + filePath + " from_line=" + std::to_string(from_line));
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.text_total_lines >= from_line + count) return;
    if (data_.loaded_text_path != filePath) {
        PERF_LOG("cache", "LoadMoreTextLines skipped (stale path)");
        return;
    }
    if (data_.text_preview.size() >= kMaxPreviewBytes) {
        PERF_LOG("cache", "LoadMoreTextLines skipped (preview exceeds " + std::to_string(kMaxPreviewBytes) + " bytes)");
        return;
    }

    std::thread([this, filePath, from_line, count]() {
        try {
            auto more = FileManager::readFileContent(filePath, from_line, from_line + count - 1);
            std::lock_guard<std::mutex> lock2(mutex_);
            if (data_.loaded_text_path != filePath) return;
            if (data_.text_preview.size() >= kMaxPreviewBytes) return;
            if (!more.empty()) {
                size_t remaining = kMaxPreviewBytes - data_.text_preview.size();
                if (more.size() > remaining) {
                    more.resize(remaining);
                }
                data_.text_preview += more;
                data_.text_total_lines = from_line + count - 1;
            }
        } catch (...) {}
    }).detach();
}

void PreviewCache::EnsureArchiveLoaded(const std::string& filePath) {
    if (!preview_pending_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.archive_loaded) return;
    data_.archive_loaded = true;
    data_.loaded_archive_path = filePath;

    std::thread([this, filePath]() {
        auto entries = ListArchiveContents(filePath);
        std::lock_guard<std::mutex> lock2(mutex_);
        if (data_.loaded_archive_path != filePath) return;
        data_.archive_contents = std::move(entries);
    }).detach();
}

void PreviewCache::SyncDirData(PreviewData& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    out.dir_contents = data_.dir_contents;
    out.dir_sorted = data_.dir_sorted;
    out.dir_loaded = data_.dir_loaded;
}

void PreviewCache::SyncTextData(PreviewData& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    out.text_preview = data_.text_preview;
    out.text_loaded = data_.text_loaded;
}

void PreviewCache::SyncArchiveData(PreviewData& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    out.archive_contents = data_.archive_contents;
    out.archive_loaded = data_.archive_loaded;
}

FTB::Editor::SyntaxHighlighter& PreviewCache::Highlighter() {
    return highlighter_;
}

void InvalidatePreviewCache() {
    PERF_LOG("cache", "InvalidatePreviewCache called");
    PreviewCache::Instance().Invalidate();
}

}
