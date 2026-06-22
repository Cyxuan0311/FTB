#include "../../include/FTB/MainUI.hpp"
#include "../../include/FTB/AsyncFileManager.hpp"
#include "../../include/FTB/FileSizeCalculator.hpp"
#include "../../include/FTB/Powerline.hpp"
#include "../../include/FTB/detail_element.hpp"
#include "../../include/FTB/StatusMessage.hpp"
#include "../../include/FTB/TextSelection.hpp"
#include "../../include/FTB/HandleManager/EventHandler.hpp"
#ifdef FTB_ENABLE_PLUGINS
#include "../../include/FTB/PluginManager.hpp"
#endif

#include <sstream>
#include <ctime>

// UI 面板
#include "../../include/UI/HelpDialog.hpp"
#include "../../include/UI/ThemeDialog.hpp"
#include "../../include/UI/LayoutPanel.hpp"
#include "../../include/UI/RenameDialog.hpp"
#include "../../include/UI/NewFileDialog.hpp"
#include "../../include/UI/NewFolderDialog.hpp"
#include "../../include/UI/FilePreviewDialog.hpp"
#include "../../include/UI/FolderDetailsDialog.hpp"
#include "../../include/UI/JumpDirectoryDialog.hpp"
#include "../../include/UI/CalendarPanel.hpp"
#include "../../include/UI/FuzzyFinderDialog.hpp"
#include "../../include/UI/DeleteDialog.hpp"
#include "../../include/UI/PluginDialog.hpp"
#include "../../include/UI/UIStylePanel.hpp"
#include "../../include/UI/SortDialog.hpp"
#ifdef FTB_ENABLE_SSH
#include "../../include/UI/SSHDialog.hpp"
#endif

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ftxui/screen/terminal.hpp>

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

// ---- 获取条目颜色 ----
Color GetEntryColor(const FileManager::DirEntryInfo& info) {
    if (info.is_dir) return TC("directory");
    if (info.is_symlink) return TC("link");
    if (info.is_executable) return TC("executable");
    return TC("file");
}

// ---- 更新路径缓存 ----
void UpdatePathCache(MainState& state) {
    try {
        fs::path canon = fs::canonical(state.currentPath);
        std::string new_canonical = canon.string();
        if (new_canonical == state.cached_canonical_path) return;

        state.cached_canonical_path = new_canonical;
        fs::path parent = canon.parent_path();
        state.cached_parent_path = (!parent.empty() && parent != canon) ? parent.string() : "/";
        state.cached_parent_display = state.cached_parent_path;

        if (!parent.empty() && parent != canon) {
            state.cached_parent_entries = FileManager::getDirectoryEntries(state.cached_parent_path);
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
        state.cached_parent_display = state.currentPath;
    }
}

// ---- 更新当前条目缓存 ----
void UpdateCurrentEntryCache(MainState& state) {
    if (state.currentPath == state.cached_current_path_for_entries) return;
    state.cached_current_path_for_entries = state.currentPath;
    state.cached_current_entries = FileManager::getDirectoryEntries(state.currentPath);
    state.allContents.clear();
    for (const auto& e : state.cached_current_entries) {
        state.allContents.push_back(e.name);
    }
}

// ---- 刷新目录内容 ----
void RefreshDirectoryContents(MainState& state) {
    state.cached_current_path_for_entries.clear();
    g_preview_cache.key.clear();
    {
        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
        FileManager::lru_dir_cache->erase(state.currentPath);
        FileManager::lru_entry_cache->erase(state.currentPath);
    }
    state.allContents = FileManager::getDirectoryContents(state.currentPath);
    state.filteredContents = state.allContents;
}

// ---- 构建面板 Modal (委托给 UI 命名空间) ----
Element BuildPanelModal(MainState& state) {
    auto term_dim = Terminal::Size();
    int tw = term_dim.dimx;
    int th = term_dim.dimy;

    switch (state.active_panel) {
    case ActivePanel::Calendar:       return UI::RenderCalendarPanel(state, tw, th);
    case ActivePanel::Help:           return UI::RenderHelpPanel(state, tw, th);
    case ActivePanel::Rename:         return UI::RenderRenamePanel(state, tw, th);
    case ActivePanel::NewFile:        return UI::RenderNewFilePanel(state, tw, th);
    case ActivePanel::NewFolder:      return UI::RenderNewFolderPanel(state, tw, th);
    case ActivePanel::DeleteConfirm:  return UI::RenderDeleteConfirmPanel(state, tw, th);
    case ActivePanel::Theme:          return UI::RenderThemePanel(state, tw, th);
    case ActivePanel::Layout:         return UI::RenderLayoutPanel(state, tw, th);
    case ActivePanel::FilePreview:    return UI::RenderFilePreviewPanel(state, tw, th);
    case ActivePanel::FolderDetails:  return UI::RenderFolderDetailsPanel(state, tw, th);
    case ActivePanel::JumpDirectory:  return UI::RenderJumpDirectoryPanel(state, tw, th);
    case ActivePanel::FuzzyFinder:    return UI::RenderFuzzyFinderPanel(state, tw, th);
    case ActivePanel::UIStyle:        return UI::RenderUIStylePanel(state, tw, th);
    case ActivePanel::StatusBarStyle: return UI::RenderStatusBarStylePanel(state, tw, th);
    case ActivePanel::Sort:           return UI::RenderSortPanel(state, tw, th);
#ifdef FTB_ENABLE_PLUGINS
    case ActivePanel::Plugin:         return UI::RenderPluginPanel(state, tw, th);
#endif
#ifdef FTB_ENABLE_SSH
    case ActivePanel::SSH:            return UI::RenderSSHPanel(state, tw, th);
#endif
    default:                          return text("");
    }
}

// ---- 处理面板事件 (委托给 UI 命名空间) ----
bool HandlePanelEvent(MainState& state, const Event& event) {
    if (state.active_panel == ActivePanel::None) return false;

    // UIStyle / StatusBarStyle panels handle all events (including Escape) internally
    switch (state.active_panel) {
    case ActivePanel::UIStyle:         return UI::HandleUIStyleEvent(state, event);
    case ActivePanel::StatusBarStyle:  return UI::HandleStatusBarStyleEvent(state, event);
    case ActivePanel::Sort:            return UI::HandleSortEvent(state, event);
    default: break;
    }

    // Escape: 关闭面板
    if (event == Event::Escape) {
        if (state.active_panel == ActivePanel::Theme) {
            auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
            FTB::ThemeManager::GetInstance()->ApplyTheme(cfg.theme.name);
        }
        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_selected = 0;
        state.panel_suggestion.clear();
        return true;
    }

    switch (state.active_panel) {
    case ActivePanel::Calendar:       return UI::HandleCalendarEvent(state, event);
    case ActivePanel::Help:           return UI::HandleHelpEvent(state, event);
    case ActivePanel::Theme:          return UI::HandleThemeEvent(state, event);
    case ActivePanel::Layout:         return UI::HandleLayoutEvent(state, event);
    case ActivePanel::Rename:         return UI::HandleRenameEvent(state, event);
    case ActivePanel::NewFile:        return UI::HandleNewFileEvent(state, event);
    case ActivePanel::NewFolder:      return UI::HandleNewFolderEvent(state, event);
    case ActivePanel::DeleteConfirm:  return UI::HandleDeleteConfirmEvent(state, event);
    case ActivePanel::FilePreview:    return UI::HandleFilePreviewEvent(state, event);
    case ActivePanel::FolderDetails:  return UI::HandleFolderDetailsEvent(state, event);
    case ActivePanel::JumpDirectory:  return UI::HandleJumpDirectoryEvent(state, event);
    case ActivePanel::FuzzyFinder:    return UI::HandleFuzzyFinderEvent(state, event);
#ifdef FTB_ENABLE_PLUGINS
    case ActivePanel::Plugin:         return UI::HandlePluginEvent(state, event);
#endif
#ifdef FTB_ENABLE_SSH
    case ActivePanel::SSH:            return UI::HandleSSHEvent(state, event);
#endif
    default:                          return true;
    }
}

// ---- 处理搜索事件 ----
bool HandleSearchEvent(MainState& state, const Event& event) {
    if (!state.search_mode) return false;

    // 让剪贴板操作穿透搜索模式
    if (event == Event::CtrlV || event == Event::CtrlY || event == Event::CtrlX) {
        return false;
    }

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
    return true;
}

// ---- 处理导航事件 ----
bool HandleNavigationEvent(MainState& state, const Event& event) {
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.selected < static_cast<int>(state.filteredContents.size()) - 1) {
            state.selected++;
            if (state.selected >= (state.current_page + 1) * state.items_per_page) {
                state.current_page = state.selected / state.items_per_page;
            }
        }
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.selected > 0) {
            state.selected--;
            if (state.selected < state.current_page * state.items_per_page) {
                state.current_page = state.selected / state.items_per_page;
            }
        }
        return true;
    }

    if (event == Event::Character('h')) {
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
        NavigateIntoSelected(state);
        return true;
    }

    if (event == Event::ArrowRight || event == Event::Return) {
        NavigateIntoSelected(state);
        return true;
    }

    if (event == Event::Home) {
        state.selected = 0;
        state.current_page = 0;
        return true;
    }

    if (event == Event::End) {
        state.selected = static_cast<int>(state.filteredContents.size()) - 1;
        if (state.selected < 0) state.selected = 0;
        state.current_page = state.selected / state.items_per_page;
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
        state.search_mode = true;
        state.searchQuery.clear();
        return true;
    }

    // Space: 切换当前项的批量选中状态
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

    return false;
}

#ifdef FTB_ENABLE_SSH
static std::string ParentPath(const std::string& path) {
    if (path == "/") return "/";
    size_t pos = path.find_last_of('/');
    if (pos == 0) return "/";
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}
#endif

#ifdef FTB_ENABLE_SSH
static bool IsRemoteDir(MainState& state, const std::string& name) {
    auto entries = FileManager::getDirectoryEntries(state.currentPath);
    for (const auto& e : entries) {
        if (e.name == name) return e.is_dir;
    }
    return false;
}
#endif

// ---- 导航到父目录 ----
void NavigateToParent(MainState& state) {
#ifdef FTB_ENABLE_SSH
    if (state.ssh_connected) {
        std::string parent = ParentPath(state.currentPath);
        if (parent != state.currentPath) {
            state.directoryHistory.push(state.currentPath);
            state.currentPath = parent;
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            g_preview_cache.key.clear();
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
        g_preview_cache.key.clear();
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

// ---- 导航进入选中项 ----
void NavigateIntoSelected(MainState& state) {
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
                g_preview_cache.key.clear();
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
            g_preview_cache.key.clear();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.selected = 0;
            state.searchQuery.clear();
            state.current_page = 0;
            state.batch_selected.clear();
        }
    }
}

// ---- 文件大小计算相关原子量 ----
static std::atomic<double> g_size_ratio(0.0);
static std::atomic<uintmax_t> g_total_folder_size(0);

// ---- 分隔符宽度 ----
int GetColumnSeparatorWidth() {
    auto& config = ConfigManager::GetInstance()->GetConfig();
    const std::string& style = config.ui.column_separator;
    if (style == "thin") return 1;
    if (style == "none") return 3;
    return 2;
}

// ---- 布局计算 ----
std::tuple<int, int, int, int> ComputeLayout() {
    auto& config = ConfigManager::GetInstance()->GetConfig();
    auto term_dim = Terminal::Size();
    int term_w = term_dim.dimx;
    int term_h = term_dim.dimy;

    int ipp = std::max(5, term_h - 4);
    if (config.layout.items_per_page > 0) {
        ipp = std::min(ipp, config.layout.items_per_page);
    }

    int sep_w = GetColumnSeparatorWidth();
    int pw = std::max(12, static_cast<int>(term_w * config.layout.parent_ratio));
    int dw = std::max(20, static_cast<int>(term_w * config.layout.preview_ratio));
    int cw = std::max(20, term_w - pw - dw - 2 * sep_w);

    return {ipp, dw, pw, cw};
}

// ---- 文件大小计算 ----
void CalculateSizes(MainState& state) {
    if (!FileSizeCalculator::IsCalculating()) {
        FileSizeCalculator::CalculateSizesAsync(state.currentPath, state.selected,
            g_total_folder_size, g_size_ratio, state.selected_size);
    }
}

// ---- 构建文件列表项 ----
Element BuildFileItem(MainState&, int, bool is_selected, bool is_hovered,
                      const std::string& name, const FileManager::DirEntryInfo& info,
                      const std::string& search_q, bool is_batch_selected = false) {
    Color text_color = GetEntryColor(info);

    auto& sel_style = ConfigManager::GetInstance()->GetConfig().ui.selection_style;

    Decorator item_style = nothing;
    if (is_selected) {
        if (sel_style == "full") {
            item_style = bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
        } else if (sel_style == "underline") {
            item_style = color(text_color) | bold | underlined;
        } else if (sel_style == "invert") {
            item_style = [](Element e) { return e | inverted | bold; };
        } else if (sel_style == "bar") {
            item_style = bgcolor(TC("selection_bg"));
        } else if (sel_style == "minimal") {
            item_style = bold;
        } else if (sel_style == "arrow" || sel_style == "rounded") {
            item_style = bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
        } else {
            item_style = bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
        }
    } else if (is_batch_selected) {
        item_style = bgcolor(TC("marker_copied")) | color(TC("main_bg"));
    } else if (is_hovered) {
        item_style = bgcolor(TC("hovered_bg"));
    }

    bool shaped_indicator = (sel_style == "arrow" || sel_style == "rounded");
    const char* indicator = "   ";
    if (is_selected && !shaped_indicator) {
        indicator = " > ";
    }

    auto build_row = [&](Element name_el) -> Element {
        if (is_selected && shaped_indicator) {
            const char* left_ch = (sel_style == "arrow") ? " \ue0b2" : " \ue0b6";
            const char* right_ch = (sel_style == "arrow") ? "\ue0b0 " : "\ue0b4 ";
            return hbox({
                text(left_ch) | color(TC("selection_bg")) | bgcolor(TC("main_bg")),
                text(" ") | bgcolor(TC("selection_bg")),
                hbox({
                    text(info.icon + " "),
                    name_el
                }) | bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold | flex,
                text(right_ch) | color(TC("selection_bg")) | bgcolor(TC("main_bg")),
            });
        }
        return hbox({
            text(indicator) | color(TC("indicator")),
            text(info.icon + " "),
            name_el
        }) | item_style | color(is_selected ? TC("selection_fg") : text_color);
    };

    if (!search_q.empty()) {
        size_t pos = name.find(search_q);
        if (pos != std::string::npos) {
            Elements parts;
            if (pos > 0)
                parts.push_back(text(name.substr(0, pos)));
            parts.push_back(text(name.substr(pos, search_q.size())) | color(TC("search_highlight")) | bold);
            if (pos + search_q.size() < name.size())
                parts.push_back(text(name.substr(pos + search_q.size())));
            return build_row(hbox(parts));
        }
    }

    return build_row(text(name));
}

// ---- 构建父目录列 ----
Element BuildParentColumn(MainState& state) {
    UpdatePathCache(state);
    const auto& parentEntriesRef = state.cached_parent_entries;

    g_parent_sel.lines.clear();

    Elements items;
    for (int i = 0; i < static_cast<int>(parentEntriesRef.size()); ++i) {
        bool is_sel = (i == state.cached_parent_selected);

        const FileManager::DirEntryInfo& entry = parentEntriesRef[i];

        std::string line_text = "  " + entry.icon + " " + entry.name;
        g_parent_sel.lines.push_back(line_text);

        bool mouse_sel = false;
        if (g_parent_sel.active) {
            int line_y = g_parent_box.y_min + i;
            int sel_y1 = std::min(g_parent_sel.anchor_y, g_parent_sel.current_y);
            int sel_y2 = std::max(g_parent_sel.anchor_y, g_parent_sel.current_y);
            if (line_y >= sel_y1 && line_y <= sel_y2) mouse_sel = true;
        }

        Color text_color = is_sel ? TC("selection_fg") : GetEntryColor(entry);

        Decorator style = nothing;
        if (is_sel || mouse_sel) {
            if (mouse_sel) {
                style = bgcolor(TC("selection_bg"));
            } else {
                auto& parent_sel = ConfigManager::GetInstance()->GetConfig().ui.selection_style;
                if (parent_sel == "full" || parent_sel == "arrow" || parent_sel == "rounded") {
                    style = bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
                } else if (parent_sel == "underline") {
                    style = color(TC("selection_fg")) | bold | underlined;
                } else if (parent_sel == "invert") {
                    style = [](Element e) { return e | inverted | bold; };
                } else if (parent_sel == "bar") {
                    style = bgcolor(TC("selection_bg"));
                } else if (parent_sel == "minimal") {
                    style = bold;
                } else {
                    style = bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
                }
            }
        }

        items.push_back(
            hbox({
                text("  "),
                text(entry.icon + " "),
                text(entry.name)
            }) | style | color(text_color)
        );
    }

    if (items.empty()) {
        items.push_back(text("  (empty)") | color(TC("dim")));
    }

    return vbox({
        text(" " + state.cached_parent_display) | color(TC("dim")) | bold,
        separator(),
        vbox(items) | flex | frame | reflect(g_parent_box)
    }) | bgcolor(TC("main_bg"));
}

// ---- 构建当前目录列 ----
Element BuildCurrentColumn(MainState& state) {
    if (!state.searchQuery.empty()) {
        state.filteredContents.clear();
        for (const auto& item : state.allContents) {
            if (item.find(state.searchQuery) != std::string::npos)
                state.filteredContents.push_back(item);
        }
        state.current_page = 0;
    } else {
        state.filteredContents = state.allContents;
    }

    state.total_pages = (static_cast<int>(state.filteredContents.size()) + state.items_per_page - 1) / state.items_per_page;
    if (state.total_pages == 0) state.total_pages = 1;
    if (state.current_page >= state.total_pages) state.current_page = state.total_pages - 1;
    if (state.current_page < 0) state.current_page = 0;

    int start_index = state.current_page * state.items_per_page;
    int end_index   = std::min(start_index + state.items_per_page, static_cast<int>(state.filteredContents.size()));

    if (state.selected < start_index || state.selected >= end_index) {
        state.current_page = state.selected / state.items_per_page;
        start_index  = state.current_page * state.items_per_page;
        end_index    = std::min(start_index + state.items_per_page, static_cast<int>(state.filteredContents.size()));
    }

    g_current_sel.lines.clear();

    auto& sel_cfg = ConfigManager::GetInstance()->GetConfig().ui.selection_style;
    bool shaped_indicator = (sel_cfg == "arrow" || sel_cfg == "rounded");

    Elements items;
    UpdateCurrentEntryCache(state);
    for (int i = start_index; i < end_index; ++i) {
        FileManager::DirEntryInfo default_info;
        const FileManager::DirEntryInfo& info = (i < static_cast<int>(state.cached_current_entries.size()))
            ? state.cached_current_entries[i] : default_info;
        const std::string& name = state.filteredContents[i];

        std::string indicator_str = (state.selected == i && !shaped_indicator) ? " > " : "   ";
        std::string line_text = indicator_str + info.icon + " " + name;
        g_current_sel.lines.push_back(line_text);

        bool mouse_sel = false;
        if (g_current_sel.active) {
            int line_y = g_current_box.y_min + (i - start_index);
            int sel_y1 = std::min(g_current_sel.anchor_y, g_current_sel.current_y);
            int sel_y2 = std::max(g_current_sel.anchor_y, g_current_sel.current_y);
            if (line_y >= sel_y1 && line_y <= sel_y2) mouse_sel = true;
        }

        bool is_batch = state.batch_selected.find(i) != state.batch_selected.end();
        auto item = BuildFileItem(state, i, state.selected == i, state.hovered_index == i,
                                  name, info, state.searchQuery, is_batch);
        if (mouse_sel) {
            item = item | bgcolor(TC("selection_bg"));
        }
        items.push_back(std::move(item));
    }

    if (items.empty()) {
        items.push_back(text("  (empty)") | color(TC("dim")));
    }

    UpdatePathCache(state);
    std::string displayPath = state.cached_canonical_path;

    return vbox({
        text(" " + displayPath) | color(TC("path")) | bold,
        separator(),
        vbox(items) | flex | frame | reflect(g_current_box)
    }) | bgcolor(TC("main_bg"));
}

// ---- 构建命令/搜索模式栏 ----
Element BuildCommandBar(MainState& state) {
    auto& keybindings = FTB::KeyBindings::GetInstance();
    bool in_prefix_mode = keybindings.IsPrefixMode();

    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    const char* right_sep = GetStatusBarSeparator(cfg.statusbar.style);

    Color status_bg = TC("status_bg");
    Color accent_bg = TC("syn_keyword");
    Color main_bg = TC("main_bg");

    std::string prompt_char = in_prefix_mode ? ":" : "/";
    std::string input_text = in_prefix_mode ? keybindings.GetCommandInput() : state.searchQuery;
    std::string hint_text = in_prefix_mode ? keybindings.GetCommandHint() : "";

    Elements cmd_elements;

    std::string mode_label = in_prefix_mode ? " COMMAND " : " SEARCH ";
    cmd_elements.push_back(
        text(mode_label) | bold | color(main_bg) | bgcolor(accent_bg)
    );
    cmd_elements.push_back(
        text(right_sep) | color(accent_bg) | bgcolor(status_bg)
    );

    cmd_elements.push_back(
        text(prompt_char) | bold | color(accent_bg) | bgcolor(status_bg)
    );

    {
        size_t cursor_pos = in_prefix_mode ? keybindings.GetCommandCursor() : input_text.size();
        if (cursor_pos > input_text.size()) cursor_pos = input_text.size();

        std::string left_text = input_text.substr(0, cursor_pos);
        std::string right_text = input_text.substr(cursor_pos);

        if (!left_text.empty()) {
            cmd_elements.push_back(
                text(left_text) | color(TC("status_fg")) | bgcolor(status_bg)
            );
        }
        cmd_elements.push_back(
            text(" ") | bgcolor(status_bg) | focusCursorBarBlinking
        );
        if (!right_text.empty()) {
            cmd_elements.push_back(
                text(right_text) | color(TC("status_fg")) | bgcolor(status_bg)
            );
        }
    }

    if (!hint_text.empty()) {
        cmd_elements.push_back(
            text(hint_text) | color(TC("dim")) | dim | bgcolor(status_bg)
        );
    }

    cmd_elements.push_back(filler() | bgcolor(status_bg));

    auto& clipboard = ClipboardManager::getInstance();
    auto clip_items = clipboard.getItems();

    auto time_now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(time_now);
    std::tm now_tm = *std::localtime(&now_c);
    std::string time_str = FileManager::formatTime(now_tm);

    std::string pos_info = std::to_string(state.selected + 1) + "/" + std::to_string(state.filteredContents.size());
    if (!clip_items.empty()) {
        pos_info += "  " + std::to_string(clip_items.size()) + " copied";
    }
    pos_info += "  " + time_str;

    cmd_elements.push_back(
        text(pos_info) | color(TC("dim")) | bgcolor(status_bg)
    );

    return hbox(cmd_elements) | bgcolor(main_bg);
}

// ---- 构建普通状态栏 ----
Element BuildNormalStatusBar(MainState& state) {
    CalculateSizes(state);

    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    const auto& sb_cfg = cfg.statusbar;
    const std::string& sb_style = sb_cfg.style;
    const char* right_sep = GetStatusBarSeparator(sb_style);
    const char* left_sep = [] (const std::string& s) -> const char* {
        if (s == "powerline")   return PowerlineChars::LeftArrow;
        if (s == "rounded")     return PowerlineChars::LeftRound;
        if (s == "thin")        return PowerlineChars::LeftArrowThin;
        if (s == "arrow")       return "\u25c2";
        if (s == "bar")         return "\u2502";
        if (s == "minimal")     return "";
        return PowerlineChars::LeftArrow;
    }(sb_style);

    auto& clipboard = ClipboardManager::getInstance();
    auto clip_items = clipboard.getItems();

    Color status_bg = TC("status_bg");
    Color accent_bg = TC("syn_keyword");
    Color main_bg = TC("main_bg");

    Elements left_segments;

    left_segments.push_back(FTB::PowerlineSegmentLeft(
        " NORMAL ", accent_bg, main_bg, status_bg, true, right_sep
    ));

    {
        std::string dir_name = fs::path(state.currentPath).filename().string();
        if (dir_name.empty()) dir_name = state.currentPath;
        left_segments.push_back(FTB::PowerlineSegmentLeft(
            " " + dir_name, status_bg, TC("status_fg"), TC("main_bg"), false, right_sep
        ));
    }

#ifdef FTB_ENABLE_SSH
    if (state.ssh_connected) {
        left_segments.push_back(FTB::PowerlineSegmentLeft(
            " SSH: " + state.ssh_label + " [" + state.ssh_remotePath + "]",
            TC("syn_keyword"), TC("main_bg"), TC("main_bg"), true, right_sep
        ));
    }
#endif

#ifdef FTB_ENABLE_PLUGINS
    {
        PluginContext pctx;
        pctx.current_path = state.currentPath;
        auto segments = PluginManager::GetInstance()->GetStatusBarContent(pctx);
        for (const auto& seg : segments) {
            Color seg_fg = TC("status_fg");
            Color seg_bg = TC("status_bg");
            if (!seg.fg_color.empty()) {
                seg_fg = ConfigManager::GetInstance()->ParseColor(seg.fg_color);
            }
            if (!seg.bg_color.empty()) {
                seg_bg = ConfigManager::GetInstance()->ParseColor(seg.bg_color);
            }
            left_segments.push_back(FTB::PowerlineSegmentLeft(
                " " + seg.text + " ", seg_bg, seg_fg, TC("main_bg"), seg.bold, right_sep
            ));
        }
    }
#endif

    Elements right_segments;

    if (sb_cfg.show_position || sb_cfg.show_clipboard) {
        std::string pos_info;
        bool has_pos = false;
        if (sb_cfg.show_position) {
            pos_info = std::to_string(state.selected + 1) + "/" + std::to_string(state.filteredContents.size());
            has_pos = true;
        }
        if (sb_cfg.show_clipboard && !clip_items.empty()) {
            if (has_pos) pos_info += "  ";
            pos_info += std::to_string(clip_items.size()) + " copied";
        }
        if (!pos_info.empty()) {
            right_segments.push_back(FTB::PowerlineSegmentRight(
                pos_info, status_bg, TC("position"), TC("main_bg"), sb_cfg.use_bold, left_sep
            ));
        }
    }

    if (sb_cfg.show_time) {
        auto time_now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(time_now);
        std::tm now_tm = *std::localtime(&now_c);
        std::string time_str = FileManager::formatTime(now_tm);
        right_segments.push_back(FTB::PowerlineSegmentRight(
            time_str, status_bg, TC("time"), TC("main_bg"), sb_cfg.use_bold, left_sep
        ));
    }

    if (cfg.style.show_permissions && state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
        const std::string& sel_name = state.filteredContents[state.selected];
        for (const auto& e : state.cached_current_entries) {
            if (e.name == sel_name && !e.permissions.empty()) {
                right_segments.push_back(FTB::PowerlineSegmentRight(
                    " " + e.permissions, status_bg, TC("syn_keyword"), TC("main_bg"), sb_cfg.use_bold, left_sep
                ));
                break;
            }
        }
    }

    auto status_msg = FTB::StatusMessage::GetCurrent();
    if (!status_msg.empty()) {
        return hbox({
            hbox(left_segments),
            filler(),
            text(" " + status_msg + " ") | color(TC("status_fg")) | bgcolor(TC("status_bg")) | bold,
            filler(),
            hbox(right_segments)
        }) | bgcolor(TC("main_bg"));
    }

    return hbox({
        hbox(left_segments),
        filler(),
        hbox(right_segments)
    }) | bgcolor(TC("main_bg"));
}

// ---- 打开编辑面板 ----
void OpenEditorForFile(MainState& state, const std::string& filePath) {
    if (state.vimEditor != nullptr) {
        FTB::NanoEditorPool::GetInstance().release(std::move(state.vimEditor));
    }
    state.vimEditor = FTB::NanoEditorPool::GetInstance().acquire();
    FTB::GlobalAsyncFileManager::getInstance().asyncReadFileContent(
        filePath, 1, 1000,
        [&state, filePath](std::string fileContent) {
            std::vector<std::string> lines;
            std::istringstream iss(fileContent);
            std::string line;
            while (std::getline(iss, line))
                lines.push_back(line);
            state.vimEditor->SetContent(lines);
            state.vimEditor->SetFilename(filePath);
            state.vimEditor->Enter();
            state.vimEditor->SetOnExit([&state, filePath](const std::vector<std::string>& content) {
                std::string contentStr;
                for (size_t i = 0; i < content.size(); ++i) {
                    contentStr += content[i];
                    if (i < content.size() - 1) contentStr += "\n";
                }
                FileManager::writeFileContent(filePath, contentStr);
                state.vim_mode_active = false;
            });
            state.vim_mode_active = true;
        });
}

// ---- 处理面板命令 ----
void HandlePanelCommand(MainState& state, FTB::KeyBindings::PanelCommand cmd) {
    switch (cmd) {
    case FTB::KeyBindings::PanelCommand::Calendar:
        state.active_panel = ActivePanel::Calendar; break;
    case FTB::KeyBindings::PanelCommand::Help:
        state.active_panel = ActivePanel::Help; break;
    case FTB::KeyBindings::PanelCommand::Theme:
        state.panel_input.clear();
        state.panel_selected = 0;
        state.theme_scroll = 0;
        state.active_panel = ActivePanel::Theme; break;
    case FTB::KeyBindings::PanelCommand::Layout:
        state.active_panel = ActivePanel::Layout; break;
    case FTB::KeyBindings::PanelCommand::Rename:
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            state.panel_input.clear();
            state.panel_message.clear();
            state.active_panel = ActivePanel::Rename;
        }
        break;
    case FTB::KeyBindings::PanelCommand::NewFile:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::NewFile; break;
    case FTB::KeyBindings::PanelCommand::NewFolder:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::NewFolder; break;
    case FTB::KeyBindings::PanelCommand::FilePreview:
        state.active_panel = ActivePanel::FilePreview; break;
    case FTB::KeyBindings::PanelCommand::FolderDetails:
        state.active_panel = ActivePanel::FolderDetails; break;
    case FTB::KeyBindings::PanelCommand::JumpDirectory:
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_selected = 0;
        state.panel_suggestion.clear();
        state.active_panel = ActivePanel::JumpDirectory; break;
    case FTB::KeyBindings::PanelCommand::FuzzyFinder:
        state.panel_input.clear();
        state.panel_selected = 0;
        state.active_panel = ActivePanel::FuzzyFinder; break;
    case FTB::KeyBindings::PanelCommand::Search:
        state.search_mode = true;
        state.searchQuery.clear();
        break;
    case FTB::KeyBindings::PanelCommand::ClearClipboard:
        ClipboardManager::getInstance().clear();
        break;
    case FTB::KeyBindings::PanelCommand::MDToggleSource: {
        g_preview_md_show_source = !g_preview_md_show_source;
        std::string msg = g_preview_md_show_source
            ? "Markdown preview: source mode"
            : "Markdown preview: rendered mode";
        // 检查当前选中文件是否为 Markdown
        if (state.selected >= 0 && state.selected < static_cast<int>(state.cached_current_entries.size())) {
            auto& entry = state.cached_current_entries[state.selected];
            std::string ext;
            auto dot = entry.name.find_last_of('.');
            if (dot != std::string::npos) {
                ext = entry.name.substr(dot);
                for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (ext != ".md" && ext != ".markdown") {
                msg += " (not a markdown file)";
            }
        }
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::Sort:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::Sort; break;
    case FTB::KeyBindings::PanelCommand::UIStyle: {
        auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
        state.panel_orig_column_sep = cfg.ui.column_separator;
        state.panel_orig_panel_border = cfg.ui.panel_border;
        state.panel_orig_selection_style = cfg.ui.selection_style;
        state.panel_selected = 0;
        state.active_panel = ActivePanel::UIStyle;
        break;
    }
    case FTB::KeyBindings::PanelCommand::StatusBarStyle: {
        auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
        state.panel_orig_statusbar_style = cfg.statusbar.style;
        state.panel_selected = 0;
        state.active_panel = ActivePanel::StatusBarStyle;
        break;
    }
#ifdef FTB_ENABLE_PLUGINS
    case FTB::KeyBindings::PanelCommand::Plugin:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::Plugin; break;
#endif
    case FTB::KeyBindings::PanelCommand::VimEdit:
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            fs::path fullPath = fs::path(state.currentPath) / state.filteredContents[state.selected];
            if (!FileManager::isDirectory(fullPath.string())) {
                OpenEditorForFile(state, fullPath.string());
            }
        }
        break;
#ifdef FTB_ENABLE_SSH
    case FTB::KeyBindings::PanelCommand::SSH:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::SSH; break;
#endif
    default:
        break;
    }
}

// ---- 注册面板命令回调 ----
void RegisterPanelCommands(FTB::KeyBindings& keybindings, MainState& state) {
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Calendar, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Calendar); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Help, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Help); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Theme, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Theme); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Layout, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Layout); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Rename, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Rename); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NewFile, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NewFile); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NewFolder, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NewFolder); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FilePreview, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FilePreview); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FolderDetails, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FolderDetails); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::JumpDirectory, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::JumpDirectory); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FuzzyFinder, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FuzzyFinder); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Search, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Search); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::VimEdit, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::VimEdit); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Sort, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Sort); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::UIStyle, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::UIStyle); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::StatusBarStyle, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::StatusBarStyle); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::ClearClipboard, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::ClearClipboard); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::MDToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::MDToggleSource); });
#ifdef FTB_ENABLE_PLUGINS
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Plugin, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Plugin); });
#endif
#ifdef FTB_ENABLE_SSH
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::SSH, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::SSH); });
#endif
}

}  // namespace FTB
