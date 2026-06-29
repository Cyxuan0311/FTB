#include "core/TabManager.hpp"
#include "core/MainUI.hpp"
#include "config/ConfigManager.hpp"
#include "preview/PreviewCache.hpp"
#include "utils/FilesystemUtil.hpp"
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace FTB {

// ---- TabClipboard ----

void TabClipboard::addItem(const std::string& path) {
    if (std::find(items.begin(), items.end(), path) == items.end()) {
        items.push_back(path);
    }
}

void TabClipboard::clear() {
    items.clear();
    cutMode = false;
    modeSelected = false;
}

void TabClipboard::setCutMode(bool mode) {
    cutMode = mode;
    modeSelected = true;
}

bool TabClipboard::paste(const std::string& targetPath) {
    if (items.empty()) return false;
    for (const auto& sourcePath : items) {
        fs::path src(sourcePath);
        fs::path dst = fs::path(targetPath) / src.filename();
        try {
            if (cutMode) {
                if (!FTB::renameWithFallback(src, dst)) return false;
            } else {
                if (fs::is_directory(src)) {
                    fs::copy(src, dst, fs::copy_options::recursive);
                } else {
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const std::exception& e) {
            return false;
        }
    }
    if (cutMode) clear();
    return true;
}

// ---- Tab ----

std::string Tab::displayName() const {
    try {
        fs::path p(currentPath);
        std::string name = p.filename().string();
        if (name.empty() || name == "/") name = "/";
        return name;
    } catch (...) {
        return "?";
    }
}

void Tab::init(const std::string& path) {
    currentPath = path;
    selected = 0;
    current_page = 0;
    searchQuery.clear();
    search_mode = false;
    batch_selected.clear();
    cached_current_path_for_entries.clear();
    cached_canonical_path.clear();
    refreshContents();
}

void Tab::refreshContents() {
    allContents = FileManager::getDirectoryContents(currentPath);
    filteredContents = allContents;
    total_pages = 1;
}

bool Tab::isValid() const {
    try {
        return fs::exists(currentPath);
    } catch (...) {
        return false;
    }
}

// ---- TabManager ----

TabManager::TabManager() {
}

int TabManager::createTab(const std::string& path) {
    Tab tab;
    tab.init(path);
    tabs_.push_back(std::move(tab));
    int newIndex = static_cast<int>(tabs_.size()) - 1;
    switchTo(newIndex);
    return newIndex;
}

bool TabManager::closeTab(int index) {
    if (tabs_.size() <= 1) return false;
    if (index < 0 || index >= static_cast<int>(tabs_.size())) return false;

    tabs_.erase(tabs_.begin() + index);

    if (activeIndex_ >= static_cast<int>(tabs_.size())) {
        activeIndex_ = static_cast<int>(tabs_.size()) - 1;
    }
    if (index <= activeIndex_ && activeIndex_ > 0) {
        activeIndex_--;
    }
    return true;
}

void TabManager::switchTo(int index) {
    if (index >= 0 && index < static_cast<int>(tabs_.size())) {
        activeIndex_ = index;
    }
}

void TabManager::nextTab() {
    if (tabs_.empty()) return;
    activeIndex_ = (activeIndex_ + 1) % tabs_.size();
}

void TabManager::prevTab() {
    if (tabs_.empty()) return;
    activeIndex_ = (activeIndex_ - 1 + static_cast<int>(tabs_.size())) % tabs_.size();
}

Tab& TabManager::active() {
    return tabs_[activeIndex_];
}

const Tab& TabManager::active() const {
    return tabs_[activeIndex_];
}

Tab& TabManager::tabAt(int index) {
    return tabs_[index];
}

const Tab& TabManager::tabAt(int index) const {
    return tabs_[index];
}

void TabManager::saveActiveTabState(MainState& state) {
    auto& tab = active();
    tab.currentPath = state.currentPath;
    tab.selected = state.selected;
    tab.hovered_index = state.hovered_index;
    tab.current_page = state.current_page;
    tab.total_pages = state.total_pages;
    tab.allContents = std::move(state.allContents);
    tab.filteredContents = std::move(state.filteredContents);
    tab.batch_selected = std::move(state.batch_selected);
    tab.searchQuery = std::move(state.searchQuery);
    tab.search_mode = state.search_mode;
    tab.cached_current_path_for_entries = std::move(state.cached_current_path_for_entries);
    tab.cached_parent_path = std::move(state.cached_parent_path);
    tab.cached_parent_display = std::move(state.cached_parent_display);
    tab.cached_canonical_path = std::move(state.cached_canonical_path);
    tab.cached_parent_selected = state.cached_parent_selected;
    tab.cached_parent_entries = std::move(state.cached_parent_entries);
    tab.cached_current_entries = std::move(state.cached_current_entries);
    tab.directoryHistory = std::move(state.directoryHistory);
}

void TabManager::loadTabState(MainState& state, int index) {
    switchTo(index);
    const auto& tab = active();
    state.currentPath = tab.currentPath;
    state.selected = tab.selected;
    state.hovered_index = tab.hovered_index;
    state.current_page = tab.current_page;
    state.total_pages = tab.total_pages;
    state.allContents = tab.allContents;
    state.filteredContents = tab.filteredContents;
    state.batch_selected = tab.batch_selected;
    state.searchQuery = tab.searchQuery;
    state.search_mode = tab.search_mode;
    state.cached_current_path_for_entries = tab.cached_current_path_for_entries;
    state.cached_parent_path = tab.cached_parent_path;
    state.cached_parent_display = tab.cached_parent_display;
    state.cached_canonical_path = tab.cached_canonical_path;
    state.cached_parent_selected = tab.cached_parent_selected;
    state.cached_parent_entries = tab.cached_parent_entries;
    state.cached_current_entries = tab.cached_current_entries;
    state.directoryHistory = tab.directoryHistory;

    if (!tab.isValid()) {
        state.currentPath = "/";
        state.cached_canonical_path.clear();
        state.cached_current_path_for_entries.clear();
        InvalidatePreviewCache();
        state.allContents = FileManager::getDirectoryContents("/");
        state.filteredContents = state.allContents;
    }
}

} // namespace FTB
