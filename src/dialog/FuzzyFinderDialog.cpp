#include "dialog/FuzzyFinderDialog.hpp"
#include "core/FuzzyFinder.hpp"
#include "config/ThemeManager.hpp"
#include "browser/FileManager.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>

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

namespace FTB::UI {

Element RenderFuzzyFinderPanel(MainState& state, int tw, int th) {
    int pw = std::min(100, tw - 4);

    std::vector<FdResult> results;
    bool loading;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        results = s_results;
        loading = s_loading;
    }

    int visible_rows = std::max(3, std::min(th - 5, 22));
    if (state.panel_selected < s_scroll) s_scroll = state.panel_selected;
    if (state.panel_selected >= s_scroll + visible_rows)
        s_scroll = state.panel_selected - visible_rows + 1;

    // === 输入行 (yazi 风格: ? 前缀) ===
    std::string input_str = state.panel_input;
    auto input_dom = hbox({
        text("? ") | color(TC("indicator")),
        text(input_str + "\u258f") | color(TC("find_keyword")),
        filler(),
    }) | bgcolor(TC("input_bg"));

    // === 文件列表 (单列, 无预览) ===
    Elements list_items;
    if (loading && results.empty()) {
        list_items.push_back(text("  searching...") | color(TC("dim")) | dim);
    } else if (results.empty() && !state.panel_input.empty()) {
        list_items.push_back(text("  no results") | color(TC("dim")) | dim);
    } else if (results.empty()) {
        list_items.push_back(text("  type to search") | color(TC("dim")) | dim);
    } else {
        int end_idx = std::min(s_scroll + visible_rows, static_cast<int>(results.size()));
        for (int i = s_scroll; i < end_idx; ++i) {
            const auto& r = results[i];
            bool selected = (i == state.panel_selected);

            auto row = hbox({
                text("  "),
                text(DisplayName(r.path)) | color(selected ? TC("selection_fg") : TC("main_fg")) | (selected ? bold : nothing),
                filler(),
                text(ParentPart(r.path)) | color(TC("dim")) | dim,
                text("  "),
            }) | (selected ? bgcolor(TC("selection_bg")) : nothing);

            list_items.push_back(row);
        }
    }

    // === 计数 / 引擎 ===
    std::string count_str;
    if (!results.empty()) {
        count_str = std::to_string(state.panel_selected + 1) + "/" + std::to_string(results.size());
    }

    auto bottom_bar = hbox({
        text(" ") | color(TC("dim")),
        text(count_str) | color(TC("dim")),
        filler(),
        text(SearchEngine::HasExternal() ? " fdfind" : " builtin") | color(TC("dim")) | dim,
        text(" "),
    });

    // === 组装: 顶部固定 + 底部填充 ===
    auto content = vbox({
        input_dom,
        separator() | color(TC("main_border")),
        vbox(std::move(list_items)) | flex,
        separator() | color(TC("main_border")),
        bottom_bar,
    }) | bgcolor(TC("main_bg")) | size(WIDTH, EQUAL, pw);

    return vbox({
        text(""),
        content,
        filler(),
    });
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
