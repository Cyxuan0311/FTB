#include <algorithm>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <thread>

#include "../include/FTB/CLIArgs.hpp"
#include "../include/FTB/MainUI.hpp"
#include "../include/FTB/ClipboardManager.hpp"
#include "../include/FTB/FileManager.hpp"
#include "../include/FTB/Powerline.hpp"
#include "../include/FTB/ThreadGuard.hpp"
#include "../include/FTB/ConfigManager.hpp"
#include "../include/FTB/ThemeManager.hpp"
#include "../include/FTB/AsyncFileManager.hpp"
#include "../include/FTB/KeyBindings.hpp"
#include "../include/FTB/detail_element.hpp"
#include "../include/FTB/StatusMessage.hpp"
#include "../include/FTB/SystemClipboard.hpp"
#include "../include/FTB/TextSelection.hpp"
#include "../include/FTB/HandleManager/EventHandler.hpp"
#ifdef FTB_ENABLE_SSH
#include "../include/UI/SSHDialog.hpp"
#endif
#ifdef FTB_ENABLE_PLUGINS
#include "../include/FTB/PluginManager.hpp"
#endif

using namespace ftxui;
namespace fs = std::filesystem;
using namespace FTB;

static void setupSignalHandlers() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGTSTP, &sa, nullptr);
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);
    sigaction(SIGTTIN, &sa, nullptr);
    sigaction(SIGTTOU, &sa, nullptr);

    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGTSTP);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGQUIT);
    sigaddset(&block_mask, SIGTTIN);
    sigaddset(&block_mask, SIGTTOU);
    sigprocmask(SIG_BLOCK, &block_mask, nullptr);
}

int main(int argc, char* argv[])
{
    setupSignalHandlers();
    auto cli_args = parse_args(argc, argv);

    if (cli_args.show_help) {
        print_help(argv[0]);
        return 0;
    }
    if (cli_args.show_version) {
        print_version();
        return 0;
    }

    // ---- 初始化 ----

    auto config_manager = FTB::ConfigManager::GetInstance();
    {
        if (!config_manager->LoadConfig(cli_args.config_path)) {
            std::cerr << "Warning: config load failed, using defaults" << std::endl;
        }
    }

    {
        FTB::ThemeManager::GetInstance();
        // 应用上次保存的主题
        const auto& saved_theme = config_manager->GetConfig().theme.name;
        if (!saved_theme.empty() && saved_theme != "default") {
            FTB::ThemeManager::GetInstance()->ApplyTheme(saved_theme);
        }
    }

#ifdef FTB_ENABLE_PLUGINS
    {
        std::string config_dir = std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.config/ftb";
        FTB::PluginManager::GetInstance()->Initialize(config_dir);
    }
#endif

    {
        FTB::GlobalAsyncFileManager::initialize();
    }

    // ---- 主状态 ----

    MainState state;
    if (!cli_args.startup_dir.empty()) {
        try {
            state.currentPath = fs::canonical(cli_args.startup_dir).string();
        } catch (...) {
            std::cerr << "Error: Invalid directory: " << cli_args.startup_dir << std::endl;
            return 1;
        }
    } else {
        state.currentPath = fs::absolute(".").string();
    }

#ifdef FTB_ENABLE_SSH
    state.ssh_records = config_manager->GetSSHRecords();
#endif

    {
        state.allContents = FileManager::getDirectoryContents(state.currentPath);
    }
    state.filteredContents = state.allContents;

    auto screen = ScreenInteractive::Fullscreen();
    state.screen = &screen;

    screen.ForceHandleCtrlZ(false);

    std::cout << "\033[?25l" << std::flush;

    // ---- UI 刷新控制 ----

    std::atomic<bool> refresh_ui{true};
    state.refresh_ui = &refresh_ui;

    std::thread timer([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    // ---- 主渲染器 ----

    auto renderer = Renderer([&] {
        auto [ipp, dw, pw, cw] = ComputeLayout();
        state.items_per_page = ipp;
        state.detail_width = dw;
        state.parent_width = pw;
        state.current_width = cw;

        auto BuildColumnSeparator = [&]() -> Element {
            auto& cfg = ConfigManager::GetInstance()->GetConfig();
            const std::string& style = cfg.ui.column_separator;
            if (style == "thin") {
                return separator() | color(TC("main_border"));
            }
            std::string sep_char;
            if (style == "light")   sep_char = "\u2502";
            else if (style == "heavy")   sep_char = "\u2503";
            else if (style == "double")  sep_char = "\u2551";
            else if (style == "dotted")  sep_char = "\u2506";
            else if (style == "dashed")  sep_char = "\u250a";
            else if (style == "none")    sep_char = " ";
            else                         sep_char = "";
            return text(" " + sep_char + " ") | color(TC("main_border"));
        };

        auto parent_col = BuildParentColumn(state) | size(WIDTH, EQUAL, state.parent_width);
        auto current_col = BuildCurrentColumn(state) | size(WIDTH, EQUAL, cw);
        auto preview_col = CreateDetailElement(state.cached_current_entries, state.selected, state.currentPath)
            | size(WIDTH, EQUAL, state.detail_width);

        auto main_content = vbox({
            hbox({
                parent_col,
                BuildColumnSeparator(),
                current_col,
                BuildColumnSeparator(),
                preview_col
            }) | flex,
            (KeyBindings::GetInstance().IsPrefixMode() || state.search_mode)
                ? BuildCommandBar(state)
                : BuildNormalStatusBar(state)
        }) | bgcolor(TC("main_bg"));

        Element result;
        if (state.vim_mode_active && state.vimEditor != nullptr) {
            result = dbox({
                main_content | dim,
                state.vimEditor->Render() | center | flex,
            });
        } else if (state.active_panel != ActivePanel::None) {
            bool no_dim = state.active_panel == ActivePanel::UIStyle
                       || state.active_panel == ActivePanel::StatusBarStyle;
            result = dbox({
                no_dim ? main_content : main_content | dim,
                BuildPanelModal(state) | center,
            });
        } else {
            result = main_content;
        }

        return result;
    });

    // ---- 注册键绑定 ----

    auto& keybindings = FTB::KeyBindings::GetInstance();
    keybindings.SetScreen(&screen);
    RegisterPanelCommands(keybindings, state);

    // ---- 事件处理 ----

    auto final_component = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Custom) {
            screen.SetCursor(Screen::Cursor{0, 0, Screen::Cursor::Hidden});
            std::cout << "\033[?25l" << std::flush;
        }

        // ---- 鼠标事件处理 (三列拖拽选择) ----
        if (event.is_mouse()) {
            auto& mouse = event.mouse();
            int sep_w = FTB::GetColumnSeparatorWidth();

            if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                auto col = GetColumnAtX(mouse.x, state.parent_width, sep_w, state.current_width);
                SelectionPress(col, mouse.x, mouse.y);
                return true;
            }
            if (mouse.button == Mouse::Left && mouse.motion == Mouse::Moved) {
                if (g_parent_sel.active || g_current_sel.active || g_preview_sel.active) {
                    SelectionDrag(mouse.x, mouse.y);
                    return true;
                }
            }
            if (mouse.button == Mouse::Left && mouse.motion == Mouse::Released) {
                std::string selected = SelectionRelease();
                if (!selected.empty()) {
                    StatusMessage::Show("Copied to clipboard");
                    std::thread([selected = std::move(selected)]() {
                        CopyToSystemClipboard(selected);
                    }).detach();
                }
                return true;
            }
        }

        if (state.vim_mode_active && state.vimEditor != nullptr) {
            if (state.vimEditor->OnEvent(event))
                return true;
        }

        if (HandlePanelEvent(state, event)) return true;
        if (HandleSearchEvent(state, event)) return true;

        if (keybindings.HandleEvent(event)) {
            return true;
        }

        if (HandleNavigationEvent(state, event)) return true;

        if (event == Event::CtrlR) {
            config_manager->ReloadConfig();
            return true;
        }

        if (event == Event::Escape) {
            if (state.active_panel != ActivePanel::None) {
            } else if (!state.searchQuery.empty()) {
                state.searchQuery.clear();
                return true;
            } else if (!state.batch_selected.empty()) {
                state.batch_selected.clear();
                return true;
            }
            return false;
        }

#ifdef FTB_ENABLE_SSH
        if (event == Event::Delete && state.ssh_connected && state.active_panel == ActivePanel::None) {
            UI::disconnectSSH(state);
            return true;
        }
#endif

        if (event == Event::CtrlC) {
            refresh_ui = false;
            screen.Exit();
            return true;
        }

        // Must return true for Ctrl+Z so FTXUI's internal HandleTask sees
        // handled=true and skips RecordSignal(SIGTSTP) which would otherwise
        // uninstall/reinstall terminal mode and cause screen flicker.
        if (event == Event::CtrlZ) {
            return true;
        }

        if (HandleFileOperationEvent(state, event)) {
            return true;
        }

        return false;
    });

    // ---- 主循环 ----

    screen.Loop(final_component);
    refresh_ui = false;

#ifdef FTB_ENABLE_SSH
    if (state.ssh_connected) {
        UI::disconnectSSH(state);
    }
#endif

    config_manager->SaveConfig();
    std::cout << "\033[?25h" << std::flush;

    return 0;
}
