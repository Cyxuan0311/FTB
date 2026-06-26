#pragma once
#include <string>
#include <vector>
#include <set>
#include "browser/DirectoryHistory.hpp"
#include "browser/SortMode.hpp"
#include "browser/FileManager.hpp"

namespace FTB {
struct MainState;

struct TabClipboard {
    std::vector<std::string> items;
    bool cutMode = false;
    bool modeSelected = false;

    void addItem(const std::string& path);
    void clear();
    bool isCutMode() const { return cutMode; }
    void setCutMode(bool mode);
    const std::vector<std::string>& getItems() const { return items; }
    bool hasModeSelected() const { return modeSelected; }
    bool paste(const std::string& targetPath);
};

struct Tab {
    std::string currentPath;
    int selected = 0;
    int hovered_index = -1;
    int current_page = 0;
    int total_pages = 1;
    std::vector<std::string> allContents;
    std::vector<std::string> filteredContents;
    std::set<int> batch_selected;
    std::string searchQuery;
    bool search_mode = false;

    SortMode sortMode = SortMode::NameAsc;
    bool useGlobalSortMode = true;

    DirectoryHistory directoryHistory;

    std::string cached_current_path_for_entries;
    std::string cached_parent_path;
    std::string cached_parent_display;
    std::string cached_canonical_path;
    int cached_parent_selected = -1;
    std::vector<FileManager::DirEntryInfo> cached_parent_entries;
    std::vector<FileManager::DirEntryInfo> cached_current_entries;

    TabClipboard clipboard;

    std::string displayName() const;
    void init(const std::string& path);
    void refreshContents();
    bool isValid() const;
};

class TabManager {
public:
    TabManager();

    int createTab(const std::string& path);
    bool closeTab(int index);
    void switchTo(int index);
    void nextTab();
    void prevTab();

    Tab& active();
    const Tab& active() const;

    Tab& tabAt(int index);
    const Tab& tabAt(int index) const;

    int activeIndex() const { return activeIndex_; }
    int count() const { return static_cast<int>(tabs_.size()); }
    bool canClose() const { return tabs_.size() > 1; }

    void saveActiveTabState(MainState& state);
    void loadTabState(MainState& state, int index);

private:
    std::vector<Tab> tabs_;
    int activeIndex_ = 0;
};

} // namespace FTB
