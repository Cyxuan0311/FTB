#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

#include <ftxui/dom/elements.hpp>

#include "editor/SyntaxHighlighter.hpp"
#include "browser/FileManager.hpp"
#include "preview/ArchivePreview.hpp"

namespace fs = std::filesystem;

namespace FTB {

static constexpr size_t kMaxPreviewBytes = 10 * 1024 * 1024;  // 10MB per preview cap

struct PreviewData {
    std::string key;
    std::string selectedName;
    bool is_dir = false;
    bool exists = false;
    bool is_symlink = false;
    bool is_executable = false;
    bool is_regular = false;
    uintmax_t file_size = 0;
    std::string mod_time;
    std::string icon;
    std::vector<FileManager::DirEntryInfo> dir_contents;
    std::string text_preview;
    std::vector<ArchiveEntry> archive_contents;
    bool dir_loaded = false;
    bool text_loaded = false;
    bool dir_sorted = false;
    bool archive_loaded = false;
    int text_total_lines = 0;
    int text_file_lines = -1;

    std::string loaded_text_path;
    std::string loaded_dir_path;
    std::string loaded_archive_path;
};

class PreviewCache {
public:
    static PreviewCache& Instance();

    PreviewData Copy();
    void Invalidate();

    void Update(const std::vector<FileManager::DirEntryInfo>& entries,
                int selected, const std::string& currentPath);
    void EnsureDirLoaded(const std::string& dirPath);
    void EnsureTextLoaded(const std::string& filePath, uintmax_t fileSize);
    void EnsureArchiveLoaded(const std::string& filePath);
    void LoadMoreTextLines(const std::string& filePath, int from_line, int count);

    void SyncDirData(PreviewData& out);
    void SyncTextData(PreviewData& out);
    void SyncArchiveData(PreviewData& out);

    FTB::Editor::SyntaxHighlighter& Highlighter();

private:
    PreviewCache() = default;

    PreviewData data_;
    std::mutex mutex_;
    FTB::Editor::SyntaxHighlighter highlighter_;
};

void InvalidatePreviewCache();

}
