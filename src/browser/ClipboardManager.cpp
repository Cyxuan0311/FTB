#include "browser/ClipboardManager.hpp"
#include "browser/TaskSystem.hpp"
#include "browser/FileManager.hpp"
#include <fstream>

ClipboardManager& ClipboardManager::getInstance() {
    static ClipboardManager instance;
    return instance;
}

void ClipboardManager::addItem(const std::string& path) {
    if (std::find(items.begin(), items.end(), path) == items.end()) {
        items.push_back(path);
    }
}

void ClipboardManager::clear() {
    items.clear();
    cutMode = false;
    modeSelected = false;
}

static fs::path resolve_dest(const fs::path& target, bool force_overwrite) {
    if (!fs::exists(target)) return target;
    if (force_overwrite) return target;

    std::string stem = target.stem().string();
    std::string ext = target.extension().string();
    auto parent = target.parent_path();

    for (int i = 1; i < 9999; ++i) {
        fs::path p = parent / (stem + " (" + std::to_string(i) + ")" + ext);
        if (!fs::exists(p)) return p;
    }
    return target;
}

struct FileEntry {
    fs::path src;
    fs::path dst;
};

struct CollectResult {
    std::vector<FileEntry> entries;
    int total_files = 0;
    uintmax_t total_bytes = 0;
};

static CollectResult collect_files(const std::vector<std::string>& items,
                                    const fs::path& base_dst,
                                    bool force_overwrite) {
    CollectResult r;
    for (const auto& item : items) {
        fs::path src(item);
        if (!fs::exists(src)) continue;

        fs::path dst = resolve_dest(base_dst / src.filename(), force_overwrite);

        if (fs::is_directory(src)) {
            r.entries.push_back({src, dst});
            r.total_files += 1;
            for (const auto& de : fs::recursive_directory_iterator(src)) {
                if (fs::is_regular_file(de.path())) {
                    std::error_code ec;
                    auto sz = fs::file_size(de.path(), ec);
                    r.total_bytes += sz;
                    r.total_files += 1;
                } else if (fs::is_directory(de.path())) {
                    r.total_files += 1;
                }
            }
        } else if (fs::is_regular_file(src)) {
            r.total_bytes += fs::file_size(src);
            r.total_files += 1;
            r.entries.push_back({src, dst});
        }
    }
    return r;
}

static bool copy_file_chunked(const fs::path& src, const fs::path& dst,
                               Progress& prog,
                               const std::atomic<bool>& cancel) {
    constexpr size_t CHUNK = 65536;
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;

    fs::create_directories(dst.parent_path());

    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;

    char buf[CHUNK];
    while (in.read(buf, CHUNK) || in.gcount() > 0) {
        if (cancel.load()) return false;
        std::streamsize n = in.gcount();
        out.write(buf, n);
        if (!out) return false;
        prog.bytes_processed += static_cast<uintmax_t>(n);
    }
    return true;
}

static bool execute_copy(const std::vector<FileEntry>& entries, bool cut_mode,
                          Progress& prog,
                          const std::atomic<bool>& cancel,
                          const std::atomic<bool>& pause) {
    for (const auto& entry : entries) {
        if (cancel.load()) return false;

        while (pause.load()) {
            if (cancel.load()) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        prog.current_file = entry.src.filename().string();
        prog.current_target = entry.dst.string();

        if (fs::is_directory(entry.src)) {
            fs::create_directories(entry.dst);
            std::error_code ec;
            for (const auto& de : fs::recursive_directory_iterator(entry.src, ec)) {
                if (cancel.load()) return false;
                if (ec) continue;

                auto rel = fs::relative(de.path(), entry.src);
                auto target = entry.dst / rel;

                if (fs::is_directory(de.path())) {
                    fs::create_directories(target);
                    ++prog.files_processed;
                } else if (fs::is_regular_file(de.path())) {
                    if (!copy_file_chunked(de.path(), target, prog, cancel))
                        return false;
                    ++prog.files_processed;
                }
            }
        } else if (fs::is_regular_file(entry.src)) {
            if (!copy_file_chunked(entry.src, entry.dst, prog, cancel))
                return false;
            ++prog.files_processed;
        }

        if (cut_mode) {
            std::error_code ec;
            fs::remove_all(entry.src, ec);
        }
    }
    return true;
}

std::string ClipboardManager::paste(const std::string& targetPath, bool force_overwrite) {
    if (items.empty()) return {};

    auto collected = collect_files(items, fs::path(targetPath), force_overwrite);

    bool is_cut = cutMode;
    std::string title = is_cut
        ? "Move " + std::to_string(items.size()) + " item(s)"
        : "Copy " + std::to_string(items.size()) + " item(s)";

    if (is_cut) {
        clear();
    }

    int total_files = collected.total_files;
    uintmax_t total_bytes = collected.total_bytes;

    auto entries = std::make_shared<std::vector<FileEntry>>(std::move(collected.entries));
    std::string dst_path = targetPath;

    TaskRequest req;
    req.title = title;
    req.type = is_cut ? TaskType::Move : TaskType::Copy;
    req.priority = Priority::Normal;
    req.work = [entries, is_cut, total_files, total_bytes,
                dst_path](TaskContext& ctx) -> bool {
        ctx.progress.total_files = total_files;
        ctx.progress.total_bytes = total_bytes;
        bool ok = execute_copy(*entries, is_cut, ctx.progress, ctx.cancel, ctx.pause);
        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
        FileManager::lru_dir_cache->erase(dst_path);
        FileManager::lru_entry_cache->erase(dst_path);
        return ok;
    };
    req.cleanup = [entries]() {
        for (const auto& e : *entries) {
            if (fs::exists(e.dst)) {
                std::error_code ec;
                fs::remove_all(e.dst, ec);
            }
        }
    };

    return TaskSystem::getInstance().submit(std::move(req));
}
