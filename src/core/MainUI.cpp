#include "core/MainUI.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unistd.h>
#include <ftxui/screen/terminal.hpp>

#include "preview/PreviewCache.hpp"
#include "browser/FileManager.hpp"
#include "config/ConfigManager.hpp"
#include "browser/SortMode.hpp"

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

void UpdatePathCache(MainState& state) {
    try {
        fs::path canon = fs::canonical(state.currentPath);
        std::string new_canonical = canon.string();
        if (new_canonical == state.cached_canonical_path) return;

        state.cached_canonical_path = new_canonical;
        fs::path parent = canon.parent_path();
        state.cached_parent_path = (!parent.empty() && parent != canon) ? parent.string() : "/";

        {
            const char* user = std::getenv("USER");
            char host[256] = {};
            if (!user) user = "unknown";
            if (::gethostname(host, sizeof(host)) != 0)
                std::strncpy(host, "unknown", sizeof(host));
            host[sizeof(host) - 1] = '\0';
            state.cached_parent_display = std::string(user) + "@" + std::string(host);
        }

        if (!parent.empty() && parent != canon) {
            state.cached_parent_entries = FileManager::getDirectoryEntries(state.cached_parent_path, state.currentSortMode());
        } else {
            state.cached_parent_entries.clear();
        }

        std::string currentDirName = canon.filename().string();
        state.cached_parent_selected = -1;
        for (int i = 0; i < static_cast<int>(state.cached_parent_entries.size()); ++i) {
            if (state.cached_parent_entries[i].name == currentDirName) {
                state.cached_parent_selected = i;
                break;
            }
        }
    } catch (...) {
        state.cached_canonical_path = state.currentPath;
        state.cached_parent_path = "";
        state.cached_parent_entries.clear();
        state.cached_parent_selected = -1;
        {
            const char* user = std::getenv("USER");
            char host[256] = {};
            if (!user) user = "unknown";
            if (::gethostname(host, sizeof(host)) != 0)
                std::strncpy(host, "unknown", sizeof(host));
            host[sizeof(host) - 1] = '\0';
            state.cached_parent_display = std::string(user) + "@" + std::string(host);
        }
    }
}

void UpdateCurrentEntryCache(MainState& state) {
    bool force_refresh = false;
    try {
        auto dir_mtime = fs::last_write_time(state.currentPath);
        if (dir_mtime != state.cached_dir_mtime) {
            force_refresh = true;
            state.cached_dir_mtime = dir_mtime;
        }
    } catch (...) {
        force_refresh = true;
    }

    if (!force_refresh && state.currentPath == state.cached_current_path_for_entries) {
        return;
    }
    state.cached_current_path_for_entries = state.currentPath;

    if (force_refresh) {
        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
        FileManager::lru_dir_cache->erase(state.currentPath);
        FileManager::lru_entry_cache->erase(state.currentPath);
    }

    state.cached_current_entries = FileManager::getDirectoryEntries(state.currentPath, state.currentSortMode());
    state.allContents.clear();
    for (const auto& e : state.cached_current_entries) {
        state.allContents.push_back(e.name);
    }

    if (!state.search_mode) {
        state.filteredContents = state.allContents;
    }
}

void RefreshDirectoryContents(MainState& state) {
    state.cached_current_path_for_entries.clear();
    InvalidatePreviewCache();
    {
        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
        FileManager::lru_dir_cache->erase(state.currentPath);
        FileManager::lru_entry_cache->erase(state.currentPath);
    }
    state.allContents = FileManager::getDirectoryContents(state.currentPath);
    state.filteredContents = state.allContents;
}

std::tuple<int, int, int, int> ComputeLayout(int tabCount) {
    auto& config = ConfigManager::GetInstance()->GetConfig();
    auto term_dim = Terminal::Size();
    int term_w = term_dim.dimx;
    int term_h = term_dim.dimy;

    int tab_bar_height = (tabCount > 1) ? 1 : 0;
    int ipp = std::max(5, term_h - 4 - tab_bar_height);
    if (config.layout.items_per_page > 0) {
        ipp = std::min(ipp, config.layout.items_per_page);
    }

    int sep_w = GetColumnSeparatorWidth();
    int pw = std::max(12, static_cast<int>(term_w * config.layout.parent_ratio));
    int dw = std::max(20, static_cast<int>(term_w * config.layout.preview_ratio));
    int cw = std::max(20, term_w - pw - dw - 2 * sep_w);

    return {ipp, dw, pw, cw};
}

bool HandleSearchEvent(MainState& state, const Event& event) {
    if (!state.search_mode) return false;

    if (event == Event::Escape) {
        state.search_mode = false;
        state.searchQuery.clear();
        return true;
    }
    if (event == Event::Return) {
        state.search_mode = false;
        return true;
    }
    if (event == Event::Backspace) {
        if (!state.searchQuery.empty()) {
            state.searchQuery.pop_back();
        } else {
            state.search_mode = false;
        }
        return true;
    }
    if (event.is_character()) {
        state.searchQuery += event.character();
        return true;
    }

    if (event == Event::ArrowUp || event == Event::ArrowDown ||
        event == Event::Home || event == Event::End ||
        event == Event::PageUp || event == Event::PageDown) {
        return false;
    }

    return true;
}

void MainState::saveTabState() {
    tabManager.saveActiveTabState(*this);
}

void MainState::loadTabState() {
    tabManager.loadTabState(*this, tabManager.activeIndex());
}

SortMode MainState::currentSortMode() const {
    const auto& tab = tabManager.active();
    if (tab.useGlobalSortMode) {
        return SortModeFromString(ConfigManager::GetInstance()->GetConfig().style.sort_mode);
    }
    return tab.sortMode;
}

}
