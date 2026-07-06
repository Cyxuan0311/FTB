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
        QuitWithCwd,
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

    // 处理主界面按键事件，返回是否消费了事件
    bool HandleEvent(const ftxui::Event& event);

    // 处理前缀模式下的命令输入
    bool HandlePrefixInput(const ftxui::Event& event);

    // 是否处于前缀模式 (Ctrl+B 已按下)
    bool IsPrefixMode() const { return prefix_mode_; }

    // 获取当前命令输入
    const std::string& GetCommandInput() const { return command_input_; }

    // 获取命令输入中的光标位置
    size_t GetCommandCursor() const { return command_cursor_; }

    // 获取当前解析的命令
    PanelCommand GetCurrentCommand() const { return current_command_; }

    // 进入/退出前缀模式
    void EnterPrefixMode();
    void ExitPrefixMode();

    // 执行当前命令，返回命令类型
    PanelCommand ExecuteCommand();

    // 获取命令输入提示
    std::string GetCommandHint() const;

    // 获取所有可用命令列表
    std::vector<std::pair<std::string, std::string>> GetCommandList() const;

    // 注册面板命令回调
    using CommandCallback = std::function<void()>;
    void RegisterCallback(PanelCommand cmd, CommandCallback callback);

    // 设置屏幕引用
    void SetScreen(ftxui::ScreenInteractive* screen) { screen_ = screen; }

    // 设置前缀键 (由配置文件传入)
    void SetPrefixKey(const std::string& key_name);

    // 获取当前前缀键名称
    const std::string& GetPrefixKeyName() const { return prefix_key_name_; }

    // 获取所有可选前缀键列表 (供帮助面板使用)
    std::vector<PrefixKeyInfo> GetAvailablePrefixKeys() const;

private:
    KeyBindings() = default;

    bool prefix_mode_ = false;
    std::string command_input_;
    size_t command_cursor_ = 0;
    PanelCommand current_command_ = PanelCommand::None;
    std::map<PanelCommand, CommandCallback> callbacks_;
    ftxui::ScreenInteractive* screen_ = nullptr;

    ftxui::Event prefix_event_ = ftxui::Event::CtrlB;
    std::string prefix_key_name_ = "CtrlB";

    // 命令名称映射
    static const std::map<std::string, PanelCommand> command_map_;
    static std::map<std::string, PanelCommand> InitCommandMap();

    static const std::map<std::string, ftxui::Event>& GetKeyEventMap();
};

} // namespace FTB

#endif // KEYBINDINGS_HPP
