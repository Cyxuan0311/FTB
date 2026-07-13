#pragma once

#include <tuple>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <atomic>
#include <functional>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include "browser/FileManager.hpp"
#include "browser/DirectoryHistory.hpp"
#include "browser/ClipboardManager.hpp"
#include "config/KeyBindings.hpp"
#include "core/TabManager.hpp"
#include "config/ConfigManager.hpp"
#include "config/ThemeManager.hpp"
#include "editor/NanoEditor.hpp"
#include "config/ObjectPool.hpp"
#include "ops/OpenerManager.hpp"
#include "renderer/detail_element.hpp"

#ifdef FTB_ENABLE_AI
#include "ai/AIAgent.hpp"
#endif

namespace FTB {

// ---- 面板状态枚举 ----
enum class ActivePanel {
    None,
    Calendar,
    Help,
    Theme,
    Layout,
    FilePreview,
    FolderDetails,
    Rename,
    NewFile,
    NewFolder,
    DeleteConfirm,
    JumpDirectory,
    FuzzyFinder,
    UIStyle,
    StatusBarStyle,
    Sort,
    NewTab,
    ShowClipboard,
    OpenerPicker,
    OpenerInput,
    OpenerConfig,
    TaskPanel,
    BatchRename,
#ifdef FTB_ENABLE_PLUGINS
    Plugin,
#endif
#ifdef FTB_ENABLE_SSH
    SSH,
#endif
#ifdef FTB_ENABLE_AI
    AI,
    AIConfig,
#endif
};

// ---- AI 面板状态 ----
#ifdef FTB_ENABLE_AI
class AIClient;
struct AILogEntry {
    enum Type { User, Assistant, Step, Success, Error, System };
    Type type;
    std::string text;
};

struct AIPanelState {
    std::string input_text;
    int input_cursor = 0;
    std::vector<AILogEntry> entries;
    bool input_focused = true;
    int log_scroll = 0;
    bool auto_scroll = true;
    int spinner_index = 0;
    int stream_visible_len = 0;
    std::string current_session_id;

    // Text selection state (line + character)
    bool sel_dragging = false;
    int sel_anchor_line = -1;
    int sel_current_line = -1;
    int sel_anchor_col = -1;
    int sel_current_col = -1;
    int conv_area_y = 3;
    int conv_area_x = 0;
    int conv_content_width = 72;

    // Input history
    std::vector<std::string> input_history;
    int history_pos = -1;
    std::string saved_input;

    // Line-based scrolling
    std::vector<int> entry_line_heights;
    int total_display_lines = 0;
    int conv_height = 0;

    // Scrollbar dragging
    bool sb_dragging = false;

    // Tool permission confirmation
    bool waiting_confirmation = false;
    ToolCall pending_tool_call;

    ~AIPanelState();
    AIPanelState();
    AIPanelState(AIPanelState&&) noexcept;
    AIPanelState& operator=(AIPanelState&&) noexcept;
    AIPanelState(const AIPanelState&) = delete;
    AIPanelState& operator=(const AIPanelState&) = delete;
};
#endif

// ---- 主界面共享状态 ----
struct MainState {
    // 布局
    int items_per_page = 20;
    int detail_width = 40;
    int parent_width = 20;
    int current_width = 40;

    // 文件列表
    std::vector<std::string> allContents;
    std::vector<std::string> filteredContents;
    int selected = 0;
    int hovered_index = -1;
    std::set<int> batch_selected;
    std::string currentPath;

    // 面板
    ActivePanel active_panel = ActivePanel::None;
    std::string panel_input;
    int panel_selected = 0;
    int theme_scroll = 0;
    std::string panel_message;
    std::string panel_suggestion;
    int help_tab = 0;  // 0=快捷键, 1=命令
    int help_scroll = 0;
    int preview_scroll_y = 0;
    int preview_scroll_x = 0;

    // Viewer tab state (ImagePreview / HexEditor)
    int viewer_scroll_y = 0;
    int viewer_scroll_x = 0;

    // Hex editor cursor state
    int hex_cursor_byte = 0;
    int hex_input_nibble = 0;  // 0=high nibble, 1=low nibble
    bool hex_modified = false;

    // 搜索
    bool search_mode = false;
    std::string searchQuery;

    // 分页
    int current_page = 0;
    int total_pages = 1;

    // 面板原始值（用于 Escape 恢复）
    std::string panel_orig_column_sep;
    std::string panel_orig_panel_border;
    std::string panel_orig_statusbar_style;
    std::string panel_orig_selection_style;
    std::string panel_orig_tab_bar_style;

    // 缓存
    std::string cached_current_path_for_entries;
    std::string cached_parent_path;
    std::string cached_parent_display;
    std::string cached_canonical_path;
    int cached_parent_selected = -1;
    std::vector<FileManager::DirEntryInfo> cached_parent_entries;
    std::vector<FileManager::DirEntryInfo> cached_current_entries;
    std::filesystem::file_time_type cached_dir_mtime;

    // 文件大小
    std::string selected_size;

    // Editor
    bool vim_mode_active = false;
    std::unique_ptr<FTB::Editor::NanoEditor> vimEditor = nullptr;

    // Opener picker state
    std::vector<std::pair<std::string, Opener>> matched_openers;
    int opener_selected = 0;
    int opener_input_mode = 0;  // 0=TUI(block), 1=GUI(orphan)

    // Batch rename state
    std::string batchrename_pattern;
    std::string batchrename_replacement;
    int batchrename_field = 0;  // 0=pattern, 1=replacement
    std::vector<std::string> batchrename_preview;

    // Opener config form state
    std::string opener_config_name;
    std::string opener_config_run;
    std::string opener_config_desc;
    std::string opener_config_rule_name;
    std::string opener_config_rule_use;
    int opener_config_field = 0;  // 0=name, 1=run, 2=desc, 3=rule_name, 4=rule_use
    bool opener_config_is_orphan = false;

    // Exit with cwd
    bool quit_with_cwd = false;
    std::string exit_path;

    // 目录历史
    DirectoryHistory directoryHistory;
    DirectoryHistory forward_history;

    // 多标签管理
    TabManager tabManager;

    void saveTabState();
    void loadTabState();
    SortMode currentSortMode() const;

    // 刷新控制
    std::atomic<bool>* refresh_ui = nullptr;
    ftxui::ScreenInteractive* screen = nullptr;

    // 计算布局回调
    std::function<std::tuple<int, int, int, int>()> computeLayout;

#ifdef FTB_ENABLE_SSH
    // SSH dialog state
    int ssh_tab = 0;            // 0=connection form, 1=records
    int ssh_field = 0;          // focused input field index
    std::string ssh_host;
    std::string ssh_port_str;
    std::string ssh_user;
    std::string ssh_pass;
    std::string ssh_remote_dir;
    // SSH connection state
    bool ssh_connected = false;
    std::string ssh_label;      // display label "user@host:port"
    std::string ssh_localPath;  // saved local path for restore on disconnect
    std::string ssh_remotePath; // current remote path
    std::vector<FTB::SSHRecord> ssh_records;
    int ssh_record_selected = 0;
#endif
#ifdef FTB_ENABLE_AI
    AIPanelState ai;
    std::unique_ptr<AIAgent> ai_agent;
    // AI config dialog state
    int ai_config_tab = 0;
    int ai_config_selected = 0;
    int ai_config_field = 0;
    bool ai_config_editing = false;
    int ai_config_active_key = 0;
    std::vector<AIKeyConfig> ai_config_keys;
#endif
};

// ---- 获取条目颜色 ----
ftxui::Color GetEntryColor(const FileManager::DirEntryInfo& info);

// ---- 构建面板 Modal ----
ftxui::Element BuildPanelModal(MainState& state);

// ---- 处理面板事件 ----
bool HandlePanelEvent(MainState& state, const ftxui::Event& event);

// ---- 处理搜索事件 ----
bool HandleSearchEvent(MainState& state, const ftxui::Event& event);

// ---- 处理导航事件 ----
bool HandleNavigationEvent(MainState& state, const ftxui::Event& event);

// ---- 刷新目录内容 ----
void RefreshDirectoryContents(MainState& state);

// ---- 更新路径缓存 ----
void UpdatePathCache(MainState& state);

// ---- 更新当前条目缓存 ----
void UpdateCurrentEntryCache(MainState& state);

// ---- 导航到父目录 ----
void NavigateToParent(MainState& state);

// ---- 导航进入选中项 ----
void NavigateIntoSelected(MainState& state);

// ---- 分隔符宽度（字符数）----
int GetColumnSeparatorWidth();

// ---- 布局计算 ----
std::tuple<int, int, int, int> ComputeLayout(int tabCount = 1);

// ---- 文件大小计算 ----
void CalculateSizes(MainState& state);

// ---- 构建文件列表项 ----
ftxui::Element BuildFileItem(MainState& state, int index, bool is_selected, bool is_hovered,
                              const std::string& name, const FileManager::DirEntryInfo& info,
                              const std::string& search_q);

// ---- 构建父目录列 ----
ftxui::Element BuildParentColumn(MainState& state);

// ---- 构建当前目录列 ----
ftxui::Element BuildCurrentColumn(MainState& state);

// ---- 构建命令/搜索模式栏 ----
ftxui::Element BuildCommandBar(MainState& state);

// ---- 构建普通状态栏 ----
ftxui::Element BuildNormalStatusBar(MainState& state);

// ---- 打开编辑面板 ----
void OpenEditorForFile(MainState& state, const std::string& filePath);
void OpenImagePreviewForFile(MainState& state, const std::string& filePath);
void OpenHexEditorForFile(MainState& state, const std::string& filePath);

// ---- 处理面板命令 ----
void HandlePanelCommand(MainState& state, FTB::KeyBindings::PanelCommand cmd);

// ---- 注册面板命令回调 ----
void RegisterPanelCommands(FTB::KeyBindings& keybindings, MainState& state);

}  // namespace FTB
