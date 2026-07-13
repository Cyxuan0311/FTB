#ifndef KEYBINDINGS_HPP
#define KEYBINDINGS_HPP

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace FTB {

struct PrefixKeyInfo {
    std::string name;
    std::string display_name;
    bool is_current = false;
    bool is_safe = true;
    std::string conflict_note;
};

/**
 * @class KeyBindings
 * @brief 统一快捷键管理模块
 *
 * 功能特性：
 * - yazi 风格快捷键 (j/k 上下, h 返回上级, l 进入目录)
 * - tmux 风格前缀键 (Ctrl+B) 用于打开面板
 * - 所有快捷键集中管理，统一调用
 */
class KeyBindings {
public:
    // 面板命令类型
    enum class PanelCommand {
        None,
        Calendar,
        Theme,
        Layout,
        JumpDirectory,
        FuzzyFinder,
        Rename,
        NewFile,
        NewFolder,
        FilePreview,
        FolderDetails,
        VimEdit,
        Help,
        Search,
        UIStyle,
        StatusBarStyle,
        Plugin,
        Sort,
        ClearClipboard,
        ShowClipboard,
        MDToggleSource,
        XLSToggleSource,
        MediaToggleSource,
        MediaPlay,
        PdfToggleSource,
        DocToggleSource,
        AudioToggleEnabled,
        HexToggleEnabled,
        GoHome,
        GoDownloads,
        GoConfig,
        NewTab,
        CloseTab,
        NextTab,
        PrevTab,
        OpenPicker,
        OpenManual,
        OpenConfig,
        Tasks,
        BatchRename,
        Extract,
        QuitWithCwd,
        PluginCommand,
        ToggleProtocol,
#ifdef FTB_ENABLE_SSH
        SSH,
#endif
#ifdef FTB_ENABLE_AI
        AI,
        AIConfig,
#endif
    };

    static KeyBindings& GetInstance();

    bool HandleEvent(const ftxui::Event& event);
    bool HandlePrefixInput(const ftxui::Event& event);
    bool IsPrefixMode() const { return prefix_mode_; }
    const std::string& GetCommandInput() const { return command_input_; }
    size_t GetCommandCursor() const { return command_cursor_; }
    PanelCommand GetCurrentCommand() const { return current_command_; }
    const std::string& GetCommandPayload() const { return command_payload_; }

    void EnterPrefixMode();
    void ExitPrefixMode();
    PanelCommand ExecuteCommand();
    std::string GetCommandHint() const;
    std::vector<std::pair<std::string, std::string>> GetCommandList() const;

    using CommandCallback = std::function<void()>;
    void RegisterCallback(PanelCommand cmd, CommandCallback callback);

    using ExternalCompleter = std::function<std::vector<std::string>(const std::string&)>;
    void SetPluginCompleter(ExternalCompleter completer) { plugin_completer_ = std::move(completer); }

    void SetScreen(ftxui::ScreenInteractive* screen) { screen_ = screen; }
    void SetPrefixKey(const std::string& key_name);
    const std::string& GetPrefixKeyName() const { return prefix_key_name_; }
    std::vector<PrefixKeyInfo> GetAvailablePrefixKeys() const;

    void LoadHistory();
    void SaveHistory();

private:
    KeyBindings() = default;

    bool prefix_mode_ = false;
    std::string command_input_;
    size_t command_cursor_ = 0;
    PanelCommand current_command_ = PanelCommand::None;
    std::string command_payload_;
    std::map<PanelCommand, CommandCallback> callbacks_;
    ftxui::ScreenInteractive* screen_ = nullptr;

    ftxui::Event prefix_event_ = ftxui::Event::CtrlB;
    std::string prefix_key_name_ = "CtrlB";

    ExternalCompleter plugin_completer_;

    // Command history
    std::vector<std::string> command_history_;
    int history_pos_ = -1;
    std::string history_saved_input_;
    static constexpr size_t kMaxHistory = 100;
    std::string history_path_;
    bool history_loaded_ = false;

    void SaveToHistory(const std::string& cmd);

    static const std::map<std::string, PanelCommand> command_map_;
    static std::map<std::string, PanelCommand> InitCommandMap();

    static const std::map<std::string, ftxui::Event>& GetKeyEventMap();
};

} // namespace FTB

#endif // KEYBINDINGS_HPP
