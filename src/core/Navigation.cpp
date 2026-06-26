#include "core/MainUI.hpp"

#include <chrono>
#include <filesystem>
#include <mutex>

#include "preview/PreviewCache.hpp"
#include "browser/FileManager.hpp"
#include "utils/StatusMessage.hpp"
#include "utils/PerfLogger.hpp"
#include "config/ConfigManager.hpp"
#include "ops/OpenerManager.hpp"

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

#ifdef FTB_ENABLE_SSH
static std::string ParentPath(const std::string& path) {
    if (path == "/") return "/";
    size_t pos = path.find_last_of('/');
    if (pos == 0) return "/";
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}

static bool IsRemoteDir(MainState& state, const std::string& name) {
    auto entries = FileManager::getDirectoryEntries(state.currentPath);
    for (const auto& e : entries) {
        if (e.name == name) return e.is_dir;
    }
    return false;
}
#endif

void NavigateToParent(MainState& state) {
    PERF_SCOPE("nav");
#ifdef FTB_ENABLE_SSH
    if (state.ssh_connected) {
        std::string parent = ParentPath(state.currentPath);
        if (parent != state.currentPath) {
            state.directoryHistory.push(state.currentPath);
            state.currentPath = parent;
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.searchQuery.clear();
            state.selected = 0;
            state.batch_selected.clear();
        }
        return;
    }
#endif

    fs::path current = fs::canonical(state.currentPath);
    if (current.has_parent_path() || !state.directoryHistory.empty()) {
        state.directoryHistory.push(state.currentPath);
        std::string newPath = current.has_parent_path()
            ? current.parent_path().string()
            : state.directoryHistory.pop();
        state.currentPath = fs::canonical(newPath).string();
        state.cached_canonical_path.clear();
        state.cached_current_path_for_entries.clear();
        InvalidatePreviewCache();
        {
            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
            FileManager::lru_dir_cache->erase(state.currentPath);
        }
        state.allContents = FileManager::getDirectoryContents(state.currentPath);
        state.filteredContents = state.allContents;
        state.searchQuery.clear();
        state.selected = 0;
        state.batch_selected.clear();
    }
}

void NavigateIntoSelected(MainState& state) {
    PERF_SCOPE("nav");
    if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
        std::string name = state.filteredContents[state.selected];

#ifdef FTB_ENABLE_SSH
        if (state.ssh_connected) {
            if (IsRemoteDir(state, name)) {
                std::string newPath = state.currentPath;
                if (newPath.back() != '/') newPath += '/';
                newPath += name;
                state.directoryHistory.push(state.currentPath);
                state.currentPath = newPath;
                state.cached_canonical_path.clear();
                state.cached_current_path_for_entries.clear();
                InvalidatePreviewCache();
                state.allContents = FileManager::getDirectoryContents(state.currentPath);
                state.filteredContents = state.allContents;
                state.selected = 0;
                state.searchQuery.clear();
                state.current_page = 0;
                state.batch_selected.clear();
            }
            return;
        }
#endif

        fs::path selectedPath = fs::path(state.currentPath) / name;
        if (FileManager::isDirectory(selectedPath.string())) {
            state.directoryHistory.push(state.currentPath);
            state.currentPath = fs::canonical(selectedPath).string();
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.selected = 0;
            state.searchQuery.clear();
            state.current_page = 0;
            state.batch_selected.clear();
        } else {
            auto& opener = OpenerManager::Instance();
            auto defaultOpener = opener.GetDefaultOpener(name);
            if (defaultOpener) {
                opener.Execute(*defaultOpener, selectedPath.string(), state.screen);
            } else {
                StatusMessage::Show("No opener configured for this file type");
            }
        }
    }
}

bool HandleNavigationEvent(MainState& state, const Event& event) {
    static auto g_last_press = std::chrono::steady_clock::now();
    static bool g_pending = false;

    if (event != Event::Character('g')) {
        g_pending = false;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        PERF_LOG("nav", "ArrowDown");
        if (state.selected < static_cast<int>(state.filteredContents.size()) - 1) {
            state.selected++;
            state.preview_scroll_y = 0;
            state.preview_scroll_x = 0;
            if (state.selected >= (state.current_page + 1) * state.items_per_page) {
                state.current_page = state.selected / state.items_per_page;
            }
        }
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        PERF_LOG("nav", "ArrowUp");
        if (state.selected > 0) {
            state.selected--;
            state.preview_scroll_y = 0;
            state.preview_scroll_x = 0;
            if (state.selected < state.current_page * state.items_per_page) {
                state.current_page = state.selected / state.items_per_page;
            }
        }
        return true;
    }

    if (event == Event::Character('h')) {
        PERF_LOG("nav", "NavigateToParent via h");
        if (state.selected != 0) { state.selected = 0; return true; }
        NavigateToParent(state);
        return true;
    }

    if (event == Event::ArrowLeft) {
        if (!state.searchQuery.empty()) { state.searchQuery.clear(); return true; }
        if (state.selected != 0) { state.selected = 0; return true; }
        NavigateToParent(state);
        return true;
    }

    if (event == Event::Character('l')) {
        PERF_LOG("nav", "NavigateIntoSelected via l");
        NavigateIntoSelected(state);
        return true;
    }

    if (event == Event::ArrowRight || event == Event::Return) {
        PERF_LOG("nav", "NavigateIntoSelected via Enter");
        NavigateIntoSelected(state);
        return true;
    }

    if (event == Event::Home) {
        PERF_LOG("nav", "Home");
        state.selected = 0;
        state.current_page = 0;
        state.preview_scroll_y = 0;
        state.preview_scroll_x = 0;
        return true;
    }

    if (event == Event::End || event == Event::Character('G')) {
        PERF_LOG("nav", "End/G");
        state.selected = static_cast<int>(state.filteredContents.size()) - 1;
        if (state.selected < 0) state.selected = 0;
        state.current_page = state.selected / state.items_per_page;
        state.preview_scroll_y = 0;
        state.preview_scroll_x = 0;
        return true;
    }

    if (event == Event::Character('g')) {
        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_press).count();
        if (g_pending && diff < 400) {
            PERF_LOG("nav", "gg");
            state.selected = 0;
            state.current_page = 0;
            state.preview_scroll_y = 0;
            state.preview_scroll_x = 0;
            g_pending = false;
            return true;
        }
        g_pending = true;
        g_last_press = now;
        return true;
    }

    if (event == Event::PageDown) {
        if (state.current_page < state.total_pages - 1) state.current_page++;
        return true;
    }

    if (event == Event::PageUp) {
        if (state.current_page > 0) state.current_page--;
        return true;
    }

    if (event == Event::Character('/')) {
        PERF_LOG("nav", "Search mode");
        state.search_mode = true;
        state.searchQuery.clear();
        return true;
    }

    if (event == Event::Character(' ')) {
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            auto it = state.batch_selected.find(state.selected);
            if (it != state.batch_selected.end())
                state.batch_selected.erase(it);
            else
                state.batch_selected.insert(state.selected);
        }
        return true;
    }

    if (event == Event::Character('.')) {
        PERF_LOG("nav", "Toggle hidden files");
        auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();
        cfg.style.show_hidden_files = !cfg.style.show_hidden_files;
        state.selected = 0;
        state.current_page = 0;
        StatusMessage::Show(cfg.style.show_hidden_files
            ? "Show hidden files: ON"
            : "Show hidden files: OFF");
        ConfigManager::GetInstance()->SaveConfig();
        return true;
    }

    return false;
}

}
