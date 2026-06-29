#include "core/MainUI.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <string>
#include <ftxui/dom/elements.hpp>

#include "utils/StatusMessage.hpp"
#include "browser/FileSizeCalculator.hpp"
#include "renderer/Powerline.hpp"
#include "browser/ClipboardManager.hpp"
#include "config/KeyBindings.hpp"
#include "browser/FileManager.hpp"
#include "browser/TaskSystem.hpp"
#include "config/ConfigManager.hpp"
#ifdef FTB_ENABLE_PLUGINS
#include "ops/PluginManager.hpp"
#endif

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

static std::atomic<double> g_size_ratio(0.0);
static std::atomic<uintmax_t> g_total_folder_size(0);

int GetColumnSeparatorWidth() {
    auto& config = ConfigManager::GetInstance()->GetConfig();
    const std::string& style = config.ui.column_separator;
    if (style == "thin") return 1;
    if (style == "none") return 3;
    return 2;
}

void CalculateSizes(MainState& state) {
    if (!FileSizeCalculator::IsCalculating()) {
        FileSizeCalculator::CalculateSizesAsync(state.currentPath, state.selected,
            g_total_folder_size, g_size_ratio, state.selected_size);
    }
}

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
            text("|") | bold | color(TC("status_fg")) | bgcolor(status_bg)
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
        std::string mode_label = clipboard.isCutMode() ? "CT" : "CP";
        Color clip_color = clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied");
        pos_info += "  ";
        cmd_elements.push_back(
            text(pos_info) | color(TC("dim")) | bgcolor(status_bg)
        );
        pos_info.clear();
        cmd_elements.push_back(
            text(mode_label + ":" + std::to_string(clip_items.size())) | color(clip_color) | bgcolor(status_bg) | bold
        );
    }
    pos_info += "  " + time_str;

    cmd_elements.push_back(
        text(pos_info) | color(TC("dim")) | bgcolor(status_bg)
    );

    return hbox(cmd_elements) | bgcolor(main_bg);
}

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

    Elements right_segments;

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
            if (seg.align == "right") {
                right_segments.push_back(FTB::PowerlineSegmentRight(
                    " " + seg.text + " ", seg_bg, seg_fg, TC("main_bg"), seg.bold, left_sep
                ));
            } else {
                left_segments.push_back(FTB::PowerlineSegmentLeft(
                    " " + seg.text + " ", seg_bg, seg_fg, TC("main_bg"), seg.bold, right_sep
                ));
            }
        }
    }
#endif

    if (sb_cfg.show_position || sb_cfg.show_clipboard) {
        std::string pos_info;
        if (sb_cfg.show_position) {
            pos_info = std::to_string(state.selected + 1) + "/" + std::to_string(state.filteredContents.size());
        }
        if (sb_cfg.show_clipboard && !clip_items.empty()) {
            std::string clip_label = clipboard.isCutMode() ? "CT:" : "CP:";
            clip_label += std::to_string(clip_items.size());
            if (!pos_info.empty()) pos_info += "  ";
            right_segments.push_back(FTB::PowerlineSegmentRight(
                pos_info, status_bg, TC("position"), TC("main_bg"), sb_cfg.use_bold, left_sep
            ));
            pos_info.clear();
            Color clip_color = clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied");
            right_segments.push_back(FTB::PowerlineSegmentRight(
                clip_label, status_bg, clip_color, TC("main_bg"), sb_cfg.use_bold, left_sep
            ));
        } else if (!pos_info.empty()) {
            right_segments.push_back(FTB::PowerlineSegmentRight(
                pos_info, status_bg, TC("position"), TC("main_bg"), sb_cfg.use_bold, left_sep
            ));
        }
    }

    {
        auto& ts = TaskSystem::getInstance();
        size_t active = ts.active_count();
        if (active > 0) {
            size_t queue = ts.queue_depth();
            std::string task_label = "Tasks: " + std::to_string(active);
            if (queue > 0) {
                task_label += "+" + std::to_string(queue);
            }
            right_segments.push_back(FTB::PowerlineSegmentRight(
                " " + task_label, status_bg, TC("syn_keyword"), TC("main_bg"), sb_cfg.use_bold, left_sep
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

}
