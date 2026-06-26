#include "dialog/FuzzyFinderDialog.hpp"
#include "core/FuzzyFinder.hpp"
#include "config/ThemeManager.hpp"
#include "browser/FileManager.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <thread>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

namespace fs = std::filesystem;
using namespace ftxui;

namespace {
    std::vector<FdResult> s_results;
    bool s_loading = false;
    int s_version = 0;
    int s_scroll = 0;
    std::mutex s_mutex;

    void ResetState() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_results.clear();
        s_loading = false;
        s_version = 0;
        s_scroll = 0;
    }

    void TriggerSearch(const std::string& query, const std::string& basePath) {
        if (query.empty()) {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_results.clear();
            s_loading = false;
            s_scroll = 0;
            return;
        }

        s_version++;
        int my_ver = s_version;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (s_loading) return;
            s_loading = true;
        }

        std::thread([query, basePath, my_ver]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                if (my_ver != s_version) { s_loading = false; return; }
            }

            auto results = SearchEngine::Search(query, basePath);

            {
                std::lock_guard<std::mutex> lock(s_mutex);
                if (my_ver == s_version) {
                    s_results = std::move(results);
                    s_scroll = 0;
                }
                s_loading = false;
            }
        }).detach();
    }
}

// ---- 获取显示名 (取最后一个路径组件) ----
static std::string DisplayName(const std::string& path) {
    auto slash = path.rfind('/');
    return (slash != std::string::npos) ? path.substr(slash + 1) : path;
}

// ---- 获取父路径部分 (用于显示) ----
static std::string ParentPart(const std::string& path) {
    auto slash = path.rfind('/');
    return (slash != std::string::npos && slash > 0) ? path.substr(0, slash) : "";
}

static std::string FmtFileSize(uintmax_t bytes) {
    char buf[32];
    if (bytes < 1024)
        std::snprintf(buf, sizeof(buf), "%zu B", bytes);
    else if (bytes < 1024 * 1024)
        std::snprintf(buf, sizeof(buf), "%.1f KB", static_cast<double>(bytes) / 1024.0);
    else
        std::snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return buf;
}

static std::string FormatFileTime(const fs::path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return "unknown";
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t t = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm;
    localtime_r(&t, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

namespace FTB::UI {

Element RenderFuzzyFinderPanel(MainState& state, int tw, int th) {
    int pw = std::min(80, tw - 6);
    int ph = std::min(30, th - 4);

    std::vector<FdResult> results;
    bool loading;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        results = s_results;
        loading = s_loading;
    }

    int visible_rows = std::max(3, ph - 8);
    if (state.panel_selected < s_scroll) s_scroll = state.panel_selected;
    if (state.panel_selected >= s_scroll + visible_rows)
        s_scroll = state.panel_selected - visible_rows + 1;

    // === 输入行 (标准模式: 标签 + 文字 + 光标) ===
    std::string input_str = state.panel_input.empty() ? "" : state.panel_input;
    auto input_dom = hbox({
        text(" Search: ") | color(TC("main_fg")),
        text(input_str + "\u258f") | color(TC("find_keyword")),
    });

    // === 左侧: 文件列表 ===
    Elements list_items;
    if (loading && results.empty()) {
        list_items.push_back(text(" (searching...)") | color(TC("dim")) | dim);
        for (int i = 1; i < visible_rows; ++i) list_items.push_back(text(""));
    } else if (results.empty() && !state.panel_input.empty()) {
        list_items.push_back(text(" (no results)") | color(TC("dim")) | dim);
        for (int i = 1; i < visible_rows; ++i) list_items.push_back(text(""));
    } else if (results.empty()) {
        list_items.push_back(text(" (type to search)") | color(TC("dim")) | dim);
        for (int i = 1; i < visible_rows; ++i) list_items.push_back(text(""));
    } else {
        int end_idx = std::min(s_scroll + visible_rows, static_cast<int>(results.size()));
        for (int i = s_scroll; i < end_idx; ++i) {
            const auto& r = results[i];
            bool selected = (i == state.panel_selected);

            auto row = hbox({
                text(selected ? " > " : "   ") | color(TC("indicator")),
                text(DisplayName(r.path)) | color(selected ? TC("selection_fg") : TC("main_fg")) | (selected ? bold : nothing),
                filler(),
                text(ParentPart(r.path)) | color(TC("dim")) | dim,
            }) | (selected ? bgcolor(TC("selection_bg")) : nothing);

            list_items.push_back(row);
        }
        int rendered = end_idx - s_scroll;
        for (int i = rendered; i < visible_rows; ++i)
            list_items.push_back(text(""));
    }

    // 标题 + 计数器 (inline, 显示在左上角)
    std::string count_str = results.empty() ? "" : (std::to_string(state.panel_selected + 1) + "/" + std::to_string(results.size()));

    auto left_panel = vbox({
        hbox({
            text(" Fd Search") | color(TC("title")) | bold,
            filler(),
            text(count_str) | color(TC("dim")),
            text("  "),
        }),
        separator() | color(TC("main_border")),
        input_dom | bgcolor(TC("input_bg")),
        separator() | color(TC("main_border")),
        vbox(std::move(list_items)) | flex,
    }) | flex;

    // === 右侧: 预览面板 ===
    Elements preview_els;
    auto add_preview = [&](const FdResult& sel) {
        fs::path fullPath = fs::path(state.currentPath) / sel.path;

        preview_els.push_back(text(" " + DisplayName(sel.path)) | color(TC("selection_fg")) | bold);
        std::string parent = ParentPart(sel.path);
        if (!parent.empty())
            preview_els.push_back(text(" " + parent) | color(TC("dim")) | dim);

        preview_els.push_back(separator() | color(TC("main_border")));

        // File type
        std::error_code ec;
        auto st = fs::status(fullPath, ec);
        if (!ec) {
            const char* type_str = "unknown";
            if (st.type() == fs::file_type::regular) type_str = "regular file";
            else if (st.type() == fs::file_type::symlink) type_str = "symlink";
            else if (st.type() == fs::file_type::character) type_str = "char device";
            else if (st.type() == fs::file_type::block) type_str = "block device";
            else if (st.type() == fs::file_type::fifo) type_str = "fifo";
            else if (st.type() == fs::file_type::socket) type_str = "socket";
            preview_els.push_back(hbox({
                text(" ") | color(TC("title")),
                text(type_str) | color(TC("main_fg")),
            }));
        }

        // File size
        auto fsize = fs::file_size(fullPath, ec);
        if (!ec) {
            preview_els.push_back(hbox({
                text(" "),
                text(FmtFileSize(fsize)) | color(TC("main_fg")),
            }));
        }

        // Modification time
        std::string ftime_str = FormatFileTime(fullPath);
        preview_els.push_back(hbox({
            text(" "),
            text(ftime_str) | color(TC("main_fg")),
        }));
    };

    if (!results.empty() && state.panel_selected >= 0 &&
        state.panel_selected < static_cast<int>(results.size())) {
        add_preview(results[state.panel_selected]);
    } else {
        preview_els.push_back(text("") | color(TC("dim")));
    }

    preview_els.push_back(filler());

    auto right_panel = vbox({
        separator() | color(TC("main_border")),
        vbox(std::move(preview_els)) | flex,
    }) | flex;

    // === 整体组装 ===
    return vbox({
        hbox({
            left_panel,
            separator() | color(TC("main_border")),
            right_panel,
        }) | flex,
        separator() | color(TC("main_border")),
        hbox({
            text(" Esc=Cancel     Arrow=Navigate     Enter=Open") | color(TC("dim")) | dim,
            filler(),
            text(SearchEngine::HasExternal() ? " fdfind" : " builtin") | color(TC("dim")) | dim,
            text(" "),
        }),
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
       size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleFuzzyFinderEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.panel_selected = 0;
        ResetState();
        return true;
    }

    if (event == Event::Return) {
        std::vector<FdResult> results;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            results = s_results;
        }

        if (state.panel_selected >= 0 && state.panel_selected < static_cast<int>(results.size())) {
            const auto& chosen = results[state.panel_selected];
            fs::path fullPath = fs::path(state.currentPath) / chosen.path;

            // 搜索只返回文件, 直接导航到父目录
            state.currentPath = fs::canonical(fullPath.parent_path()).string();

            // 强制清除所有缓存, 确保读取最新目录内容
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_dir_cache->erase(state.currentPath);
                FileManager::lru_entry_cache->erase(state.currentPath);
            }
            // 用 getDirectoryEntries 读取(与 BuildCurrentColumn/UpdateCurrentEntryCache 排序一致)
            auto entries = FileManager::getDirectoryEntries(state.currentPath);
            state.allContents.clear();
            state.allContents.reserve(entries.size());
            for (const auto& e : entries)
                state.allContents.push_back(e.name);
            state.filteredContents = state.allContents;
            state.searchQuery.clear();
            state.batch_selected.clear();

            // 选中目标文件
            auto filename = fs::path(chosen.path).filename().string();
            auto it = std::find(state.filteredContents.begin(), state.filteredContents.end(), filename);
            if (it != state.filteredContents.end()) {
                state.selected = static_cast<int>(std::distance(state.filteredContents.begin(), it));
                state.current_page = state.selected / state.items_per_page;
            } else {
                state.selected = 0;
                state.current_page = 0;
            }
        }

        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.panel_selected = 0;
        ResetState();
        return true;
    }

    if (event == Event::ArrowUp) {
        if (state.panel_selected > 0) state.panel_selected--;
        if (state.panel_selected < s_scroll) s_scroll = state.panel_selected;
        return true;
    }

    if (event == Event::ArrowDown) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (state.panel_selected + 1 < static_cast<int>(s_results.size())) {
            state.panel_selected++;
            if (state.panel_selected >= s_scroll + 1)
                s_scroll = std::max(0, state.panel_selected);
        }
        return true;
    }

    // 字符输入 (包括 j/k 都作为搜索文字)
    if (event.is_character()) {
        state.panel_input += event.character();
        state.panel_selected = 0;
        TriggerSearch(state.panel_input, state.currentPath);
        return true;
    }

    if (event == Event::Backspace) {
        if (!state.panel_input.empty()) {
            state.panel_input.pop_back();
            state.panel_selected = 0;
            TriggerSearch(state.panel_input, state.currentPath);
        }
        return true;
    }

    return true;
}

}  // namespace FTB::UI
