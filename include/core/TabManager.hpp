#pragma once
#include <string>
#include <vector>
#include <set>
#include <memory>
#include "browser/DirectoryHistory.hpp"
#include "browser/SortMode.hpp"
#include "browser/FileManager.hpp"

namespace FTB {
struct MainState;

namespace Editor { class NanoEditor; }

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

enum class TabType {
    FileBrowser,
    ImagePreview,
    HexEditor,
#ifdef FTB_ENABLE_AI
    AIAgent,
#endif
    Editor,
};

struct Tab {
    TabType type = TabType::FileBrowser;
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

    std::string viewer_filepath;
    int viewer_scroll_y = 0;
    int viewer_scroll_x = 0;
    int hex_cursor_byte = 0;
    int hex_input_nibble = 0;

#ifdef FTB_ENABLE_AI
    std::string ai_session_id;
#endif
    std::unique_ptr<FTB::Editor::NanoEditor> editor;
    std::string editor_filepath;
    std::string display_name_override;

    std::string displayName() const;
    void init(const std::string& path);
    void refreshContents();
    bool isValid() const;
};

class TabManager {
public:
    TabManager();

    int createTab(const std::string& path);
    int createImageTab(const std::string& path, const std::string& filepath);
    int createHexTab(const std::string& path, const std::string& filepath);
#ifdef FTB_ENABLE_AI
    int createAITab(const std::string& path, const std::string& session_id, const std::string& display_name);
#endif
    int createEditorTab(const std::string& path, const std::string& filepath, std::unique_ptr<FTB::Editor::NanoEditor> editor);
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
    bool isEditorTab(int index) const;
    bool isImageTab(int index) const;
    bool isHexTab(int index) const;
    int editorTabCount() const;
#ifdef FTB_ENABLE_AI
    bool isAITab(int index) const;
    int aiTabCount() const;
#endif

    void saveActiveTabState(MainState& state);
    void loadTabState(MainState& state, int index);

private:
    std::vector<Tab> tabs_;
    int activeIndex_ = 0;
};

} // namespace FTB
