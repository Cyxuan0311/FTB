#include <algorithm>
#include <csignal>
#include <memory>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <sstream>
#include <thread>

#include "../include/config/CLIArgs.hpp"
#include "../include/core/MainUI.hpp"
#include "../include/browser/ClipboardManager.hpp"
#include "../include/browser/FileManager.hpp"
#include "../include/renderer/Powerline.hpp"
#include "../include/config/ThreadGuard.hpp"
#include "../include/config/ConfigManager.hpp"
#include "../include/config/ThemeManager.hpp"
#include "../include/browser/AsyncFileManager.hpp"
#include "../include/browser/TaskSystem.hpp"
#include "../include/config/KeyBindings.hpp"
#include "../include/renderer/detail_element.hpp"
#include "../include/utils/StatusMessage.hpp"
#include "../include/utils/PerfLogger.hpp"
#include "../include/protocols/ImageOutputManager.hpp"
#include "../include/utils/SystemClipboard.hpp"
#include "../include/renderer/TextSelection.hpp"
#ifdef FTB_ENABLE_AI
#include "../include/ai/SessionManager.hpp"
#include "../include/dialog/AIDialog.hpp"
#endif
#include "../include/dialog/EditorPanel.hpp"
#include "../include/dialog/ImagePreviewPanel.hpp"
#include "../include/dialog/HexEditorPanel.hpp"
#include "../include/ops/EventHandler.hpp"
#include "../include/dialog/TabBarRenderer.hpp"
#include "../include/dialog/OpenerPickerDialog.hpp"
#include "../include/dialog/OpenerInputDialog.hpp"
#ifdef FTB_ENABLE_SSH
#include "../include/dialog/SSHDialog.hpp"
#endif
#ifdef FTB_ENABLE_PLUGINS
#include "../include/ops/PluginManager.hpp"
#endif

using namespace ftxui;
namespace fs = std::filesystem;
using namespace FTB;

// Node wrapper that post-processes the Screen after child rendering:
// replaces characters in the image area with cursor-forward escapes,
// preventing Screen::Print from writing spaces (which clear some protocols).
class ImageSkipNode : public ftxui::Node {
    ftxui::Element child_;
public:
    ImageSkipNode(ftxui::Element child) : child_(std::move(child)) {}

    void ComputeRequirement() override {
        child_->ComputeRequirement();
        requirement_ = child_->requirement();
    }

    void SetBox(ftxui::Box box) override {
        ftxui::Node::SetBox(box);
        child_->SetBox(box);
    }

    void Render(ftxui::Screen& screen) override {
        child_->Render(screen);
        auto* proto = FTB::ImageOutputManager::ActiveProtocol();
        if (!proto || !proto->NeedsSkipArea()) return;
        int r = FTB::ImageOutputManager::CurrentRow();
        int c = FTB::ImageOutputManager::CurrentCol();
        int h = FTB::ImageOutputManager::CurrentImageRows();
        if (r <= 0 || h <= 0 || c < 0) return;
        int max_rows = FTB::ImageOutputManager::CurrentMaxRows();
        if (max_rows > 0) h = std::min(h, max_rows);
        int max_y = std::min(r + h, screen.dimy());
        for (int y = r; y < max_y; y++) {
            for (int x = c; x < screen.dimx(); x++) {
                screen.PixelAt(x, y).character = "\033[C";
            }
        }
    }
};

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
    std::cout << "\033[?25l" << std::flush;

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
        auto& kb = FTB::KeyBindings::GetInstance();
        kb.SetPrefixKey(config_manager->GetConfig().keybindings.prefix_key);
    }

    {
        FTB::OpenerManager::Instance().LoadConfig(config_manager->GetConfig().opener);
    }

    {
        FTB::ThemeManager::GetInstance();
        // 应用上次保存的主题
        const auto& saved_theme = config_manager->GetConfig().theme.name;
        if (!saved_theme.empty() && saved_theme != "default") {
            FTB::ThemeManager::GetInstance()->ApplyTheme(saved_theme);
        }
    }

#ifdef FTB_ENABLE_AI
    {
        const char* home = getenv("HOME");
        std::string sessions_dir = home ? (std::string(home) + "/.config/ftb") : "/tmp/.config/ftb";
        std::string sessions_path = sessions_dir + "/ai_sessions.json";
        std::filesystem::create_directories(sessions_dir);
        FTB::SessionManager::getInstance().load(sessions_path);
    }
#endif

    {
        if (cli_args.log_enabled) {
            FTB::PerfLogger::GetInstance().Enable();
        }
        PERF_LOG("Init", "PerfLogger enabled=" + std::to_string(cli_args.log_enabled));
    }

#ifdef FTB_ENABLE_PLUGINS
    {
        std::string config_dir = std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/.config/ftb";
        FTB::PluginManager::GetInstance()->Initialize(config_dir);

        PERF_LOG("Init", "Plugin initialization complete");
    }
#endif

    {
        FTB::GlobalAsyncFileManager::initialize();
    }

    {
        TaskSystem::getInstance(); // Ensure singleton is constructed
    }

    {
        FTB::ImageOutputManager::SetProtocolEnabled(config_manager->GetConfig().preview.protocol_enabled);
        FTB::ImageOutputManager::DetectProtocol();
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

    state.tabManager.createTab(state.currentPath);

    auto screen = ScreenInteractive::Fullscreen();
    state.screen = &screen;
    screen.SetCursor(Screen::Cursor{0, 0, Screen::Cursor::Hidden});

    screen.ForceHandleCtrlZ(false);

    // ---- Start background plugin refresh ----

#ifdef FTB_ENABLE_PLUGINS
    {
        auto pm = FTB::PluginManager::GetInstance();
        pm->StartBackgroundRefresh([&screen]() {
            screen.Post(Event::Custom);
        });
    }
#endif

    // ---- UI 刷新控制 ----

    std::atomic<bool> refresh_ui{true};
    state.refresh_ui = &refresh_ui;

    // ---- UI refresh timer (150ms) ----
    // Only posts Event::Custom. The image flush is handled
    // synchronously in the CatchEvent handler on the main
    // thread, right after Screen::Print() completes.
    std::thread timer([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    // ---- 主渲染器 ----

    auto renderer = Renderer([&] {
        auto [ipp, dw, pw, cw] = ComputeLayout(state.tabManager.count());
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
        int preview_idx = state.selected;
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            const std::string& sel_name = state.filteredContents[state.selected];
            for (size_t ei = 0; ei < state.cached_current_entries.size(); ++ei) {
                if (state.cached_current_entries[ei].name == sel_name) {
                    preview_idx = static_cast<int>(ei);
                    break;
                }
            }
        }
        auto preview_col = CreateDetailElement(state.cached_current_entries, preview_idx, state.currentPath,
                                               state.preview_scroll_y, state.preview_scroll_x)
            | size(WIDTH, EQUAL, state.detail_width);

        auto tab_bar = UI::BuildTabBar(state);

        auto term_dim = Terminal::Size();
        int tw = term_dim.dimx;
        int th = term_dim.dimy;

        Element main_body;
        if (state.tabManager.active().type == TabType::ImagePreview) {
            main_body = UI::RenderImagePreviewPanel(state, tw, th) | flex;
        } else
        if (state.tabManager.active().type == TabType::HexEditor) {
            main_body = UI::RenderHexEditorPanel(state, tw, th) | flex;
        } else
        if (state.tabManager.active().type == TabType::Editor) {
            auto& tab = state.tabManager.active();
            if (tab.editor) {
                main_body = UI::RenderEditorPanel(state, tw, th) | flex;
            } else {
                main_body = text("Editor loading...") | center;
            }
        } else
#ifdef FTB_ENABLE_AI
        if (state.tabManager.active().type == TabType::AIAgent) {
            main_body = UI::RenderAIPanel(state, tw, th, true) | flex;
        } else
#endif
        {
            main_body = hbox({
                parent_col,
                BuildColumnSeparator(),
                current_col,
                BuildColumnSeparator(),
                preview_col
            }) | flex;
        }

        auto main_content = vbox({
            tab_bar,
            main_body,
            (KeyBindings::GetInstance().IsPrefixMode() || state.search_mode)
                ? BuildCommandBar(state)
                : BuildNormalStatusBar(state)
        }) | bgcolor(TC("main_bg"));

        // Sync overlay state so sixel image doesn't cover popup dialogs
        FTB::ImageOutputManager::SetOverlayActive(state.active_panel != ActivePanel::None);

        Element result;
        if (state.active_panel != ActivePanel::None) {
            bool no_dim = state.active_panel == ActivePanel::UIStyle
                       || state.active_panel == ActivePanel::StatusBarStyle;
            result = dbox({
                no_dim ? main_content : main_content | dim,
                BuildPanelModal(state) | center,
            });
        } else {
            result = main_content;
        }

        auto* proto = FTB::ImageOutputManager::ActiveProtocol();
        if (proto && proto->NeedsSkipArea()) {
            return ftxui::Element(std::make_shared<ImageSkipNode>(std::move(result)));
        }
        return result;
    });

    // ---- 注册键绑定 ----

    auto& keybindings = FTB::KeyBindings::GetInstance();
    keybindings.SetScreen(&screen);
    RegisterPanelCommands(keybindings, state);

#ifdef FTB_ENABLE_PLUGINS
    keybindings.SetPluginCompleter([](const std::string& input) {
        return FTB::PluginManager::GetInstance()->CompletePluginCommand(input);
    });
#endif

    // ---- 事件处理 ----

    auto final_component = CatchEvent(renderer, [&](Event event) {
        if (event != Event::Custom) {
            PERF_LOG("EventTrace", event.DebugString());
        }
        if (event == Event::Custom) {
            screen.SetCursor(Screen::Cursor{0, 0, Screen::Cursor::Hidden});
            if (state.refresh_pending.exchange(false)) {
                RefreshDirectoryContents(state);
                if (state.selected >= static_cast<int>(state.filteredContents.size()))
                    state.selected = std::max(0, static_cast<int>(state.filteredContents.size()) - 1);
            }
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            static auto last_custom = std::chrono::steady_clock::now();
            static bool deferred_flush_pending = false;
            auto now = std::chrono::steady_clock::now();
            auto since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_custom).count();
            last_custom = now;

            // Gate: skip fast-repeat RENDER events (PluginManager bursts)
            // to reduce redundant Print→FLUSH cycles. FLUSH follow-ups
            // are never gated.
            if (!deferred_flush_pending) {
                static bool first_custom = true;
                if (!first_custom && since_last < 50) {
                    return false;
                }
                first_custom = false;
            }

            if (deferred_flush_pending) {
                // Follow-up event: flush image AFTER Screen::Print
                // has completed, then skip re-render so Print
                // does not overwrite the image again.
                deferred_flush_pending = false;
                FTB::ImageOutputManager::FlushPendingIfDirty();
                return false;
            } else {
                // First event: post a follow-up event, then let
                // FTXUI run Render + Screen::Print().
                deferred_flush_pending = true;
                screen.Post(Event::Custom);
                return true;
            }
        }

        // ---- 鼠标事件处理 (三列拖拽选择 + 标签栏点击) ----
        if (event.is_mouse()) {
            auto& mouse = event.mouse();

            // Tab bar click (y == 0)
            if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed && mouse.y == 0) {
                int tabIdx = UI::GetTabAtX(mouse.x);
                if (tabIdx >= 0 && tabIdx != state.tabManager.activeIndex()) {
                    state.saveTabState();
                    state.tabManager.switchTo(tabIdx);
                    state.loadTabState();
                }
                return true;
            }

#ifdef FTB_ENABLE_AI
            bool is_ai_active = (state.active_panel == ActivePanel::AI)
                             || (state.tabManager.active().type == TabType::AIAgent);
            if (is_ai_active) {
                if (mouse.button == Mouse::WheelUp) {
                    state.ai.log_scroll = std::max(0, state.ai.log_scroll - 3);
                    state.ai.auto_scroll = false;
                    return true;
                }
                if (mouse.button == Mouse::WheelDown) {
                    int max_scroll = std::max(0, state.ai.total_display_lines - state.ai.conv_height);
                    state.ai.log_scroll = std::min(max_scroll, state.ai.log_scroll + 3);
                    state.ai.auto_scroll = (state.ai.log_scroll >= max_scroll);
                    return true;
                }

                if (mouse.button == Mouse::Left) {
                    // Scrollbar drag (right side of conversation)
                    int sb_x = state.ai.conv_area_x + state.ai.conv_content_width;
                    bool on_scrollbar = (mouse.x >= sb_x && mouse.x < sb_x + 2);

                    if (on_scrollbar && (mouse.motion == Mouse::Pressed || mouse.motion == Mouse::Moved)) {
                        int max_scroll = std::max(0, state.ai.total_display_lines - state.ai.conv_height);
                        int rel_y = mouse.y - state.ai.conv_area_y;
                        int track_h = state.ai.conv_height;
                        int target = (track_h > 1) ? max_scroll * rel_y / (track_h - 1) : 0;
                        state.ai.log_scroll = std::clamp(target, 0, max_scroll);
                        state.ai.auto_scroll = (state.ai.log_scroll >= max_scroll);
                        if (mouse.motion == Mouse::Pressed) state.ai.sb_dragging = true;
                        return true;
                    }
                    if (mouse.motion == Mouse::Released && state.ai.sb_dragging) {
                        state.ai.sb_dragging = false;
                        return true;
                    }

                    // Text selection
                    if (mouse.motion == Mouse::Pressed) {
                        state.ai.sel_dragging = true;
                        int abs_line = state.ai.log_scroll + mouse.y - state.ai.conv_area_y;
                        state.ai.sel_anchor_line = abs_line;
                        state.ai.sel_current_line = abs_line;
                        int abs_col = mouse.x - state.ai.conv_area_x;
                        state.ai.sel_anchor_col = abs_col;
                        state.ai.sel_current_col = abs_col;
                        return true;
                    }
                    if (mouse.motion == Mouse::Moved && state.ai.sel_dragging) {
                        int abs_line = state.ai.log_scroll + mouse.y - state.ai.conv_area_y;
                        state.ai.sel_current_line = abs_line;
                        int abs_col = mouse.x - state.ai.conv_area_x;
                        state.ai.sel_current_col = abs_col;
                        return true;
                    }
                    if (mouse.motion == Mouse::Released && state.ai.sel_dragging) {
                        state.ai.sel_dragging = false;
                        auto sel_text = FTB::UI::ExtractAISelectedText(state);
                        if (!sel_text.empty()) {
                            StatusMessage::Show("Copied to clipboard");
                            std::thread([t = std::move(sel_text)]() {
                                CopyToSystemClipboard(t);
                            }).detach();
                        }
                        return true;
                    }
                }
                // Don't fall through to column selection if AI panel is active
                return true;
            }
#endif

            // Column selection and preview scroll only apply in file browser mode
            if (state.tabManager.active().type == TabType::FileBrowser) {
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
                if (mouse.button == Mouse::WheelUp) {
                    if (state.preview_scroll_y > 0)
                        state.preview_scroll_y = std::max(0, state.preview_scroll_y - 3);
                    return true;
                }
                if (mouse.button == Mouse::WheelDown) {
                    state.preview_scroll_y += 3;
                    return true;
                }
            }
        }

        if (state.tabManager.active().type == TabType::ImagePreview) {
            if (UI::HandleImagePreviewEvent(state, event))
                return true;
        } else
        if (state.tabManager.active().type == TabType::HexEditor) {
            if (UI::HandleHexEditorEvent(state, event))
                return true;
        } else
        if (state.tabManager.active().type == TabType::Editor) {
            if (UI::HandleEditorEvent(state, event))
                return true;
        }

        if (HandlePanelEvent(state, event)) return true;
        if (HandleSearchEvent(state, event)) return true;

        if (keybindings.HandleEvent(event)) {
            return true;
        }

        // Tab switching (only when no panel/search active)
        if (state.active_panel == ActivePanel::None && !state.search_mode) {
            if (event == Event::Character(']')) {
                state.saveTabState();
                state.tabManager.nextTab();
                state.loadTabState();
                return true;
            }
            if (event == Event::Character('[')) {
                state.saveTabState();
                state.tabManager.prevTab();
                state.loadTabState();
                return true;
            }
        }

        if (HandleNavigationEvent(state, event)) return true;

        // Preview panel scroll shortcuts
        if (state.active_panel == ActivePanel::None && !state.search_mode && state.tabManager.active().type != TabType::Editor) {
            if (event == Event::AltJ) {
                state.preview_scroll_y += 3;
                return true;
            }
            if (event == Event::AltK) {
                if (state.preview_scroll_y > 0)
                    state.preview_scroll_y = std::max(0, state.preview_scroll_y - 3);
                return true;
            }
            if (event == Event::AltH) {
                if (state.preview_scroll_x > 0)
                    state.preview_scroll_x = std::max(0, state.preview_scroll_x - 5);
                return true;
            }
            if (event == Event::AltL) {
                state.preview_scroll_x += 5;
                return true;
            }
        }

        if (event == Event::CtrlR) {
            config_manager->ReloadConfig();
            FTB::KeyBindings::GetInstance().SetPrefixKey(config_manager->GetConfig().keybindings.prefix_key);
            FTB::StatusMessage::Show("Config reloaded");
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

#ifdef FTB_ENABLE_PLUGINS
    FTB::PluginManager::GetInstance()->StopBackgroundRefresh();
#endif

#ifdef FTB_ENABLE_SSH
    if (state.ssh_connected) {
        UI::disconnectSSH(state);
    }
#endif

#ifdef FTB_ENABLE_AI
    {
        if (state.ai_agent) {
            state.ai_agent->syncToSessionManager();
        }
        FTB::SessionManager::getInstance().save(
            FTB::SessionManager::getInstance().filePath());
    }
#endif

    config_manager->SaveConfig();
    std::cout << "\033[?25h" << std::flush;

    // 清理终端图像残留（Kitty/iTerm2/Sixel）
    FTB::ImageOutputManager::ClearCurrent();

    if (state.quit_with_cwd && !state.exit_path.empty()) {
        const char* cwd_file_env = std::getenv("FTB_CWD_FILE");
        std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
        std::string cwd_file = cwd_file_env
            ? cwd_file_env
            : (home + "/.cache/ftb/cwd");
        try {
            std::error_code ec;
            fs::create_directories(fs::path(cwd_file).parent_path(), ec);
            if (!ec) {
                std::ofstream ofs(cwd_file);
                if (ofs) ofs << state.exit_path;
            }
        } catch (...) {
        }
    }

    return 0;
}
