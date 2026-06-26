#include "dialog/TabBarRenderer.hpp"
#include "core/TabManager.hpp"
#include "config/ConfigManager.hpp"
#include "renderer/Powerline.hpp"
#include <algorithm>

namespace FTB::UI {

using namespace ftxui;

static std::vector<int> g_tab_x_starts;

Element BuildTabBar(MainState& state) {
    auto& tabs = state.tabManager;
    int n = tabs.count();
    if (n <= 1) return text("");

    g_tab_x_starts.clear();

    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();
    const std::string& style = cfg.ui.tab_bar_style;

    Color main_bg          = TC("main_bg");
    Color border_col       = TC("main_border");
    Color tab_active_bg    = TC("tab_active_bg");
    Color tab_active_fg    = TC("tab_active_fg");
    Color tab_inactive_fg  = TC("tab_inactive_fg");

    int active_idx = tabs.activeIndex();
    Elements segments;
    int current_x = 0;

    for (int i = 0; i < n; ++i) {
        bool is_active = (i == active_idx);
        auto& tab = tabs.tabAt(i);

        std::string label = tab.displayName();
        if (label.size() > 16) {
            label = label.substr(0, 14) + "...";
        }

        g_tab_x_starts.push_back(current_x);

        Element tab_element;
        if (is_active) {
            if (style == "classic") {
                std::string content = " " + label + " ";
                tab_element = text(content) | bgcolor(tab_active_bg) | color(tab_active_fg) | bold;
                current_x += content.size();
            } else if (style == "rounded") {
                std::string content = " " + label + " ";
                int cw = content.size();
                tab_element = hbox({
                    text(PowerlineChars::LeftRound) | color(tab_active_bg) | bgcolor(main_bg),
                    text(content) | color(tab_active_fg) | bgcolor(tab_active_bg) | bold,
                    text(PowerlineChars::RightRound) | color(tab_active_bg) | bgcolor(main_bg),
                });
                current_x += cw + 2;
            } else if (style == "underline") {
                std::string content = " " + label + " ";
                tab_element = text(content) | color(tab_active_fg) | bgcolor(main_bg) | bold | underlined;
                current_x += content.size();
            } else if (style == "elegant") {
                std::string content = " " + label + " ";
                int cw = content.size();
                tab_element = hbox({
                    text("\u251c ") | color(tab_active_bg) | bgcolor(main_bg),
                    text(content) | color(tab_active_fg) | bgcolor(tab_active_bg) | bold,
                    text(" \u2524") | color(tab_active_bg) | bgcolor(main_bg),
                });
                current_x += cw + 4;
            } else if (style == "minimal") {
                std::string content = " " + label + " ";
                tab_element = hbox({
                    text(" \u25cf ") | color(tab_active_bg) | bgcolor(main_bg),
                    text(label) | color(tab_active_fg) | bgcolor(main_bg) | bold,
                    text(" ") | bgcolor(main_bg),
                });
                current_x += content.size() + 1;
            } else {
                // modern (default): half-block powerline
                std::string content = " " + label + " ";
                int cw = content.size();
                tab_element = hbox({
                    text("\u2590") | color(tab_active_bg) | bgcolor(main_bg),
                    text(content) | color(tab_active_fg) | bgcolor(tab_active_bg) | bold,
                    text("\u258c") | color(tab_active_bg) | bgcolor(main_bg),
                });
                current_x += cw + 2;
            }
        } else {
            std::string content = " " + label + " ";
            if (style == "minimal") {
                tab_element = hbox({
                    text("   ") | bgcolor(main_bg),
                    text(label) | color(tab_inactive_fg) | bgcolor(main_bg),
                    text(" ") | bgcolor(main_bg),
                });
                current_x += content.size() + 1;
            } else {
                tab_element = text(content) | color(tab_inactive_fg) | bgcolor(main_bg);
                current_x += content.size();
            }
        }

        if (i < n - 1) {
            Decorator sep_style = color(border_col) | bgcolor(main_bg);
            tab_element = hbox({
                tab_element,
                text(" \u2502 ") | sep_style,
            });
            current_x += 3;
        }

        segments.push_back(tab_element);
    }

    segments.push_back(filler() | bgcolor(main_bg));

    auto bar = hbox(std::move(segments)) | bgcolor(main_bg);
    auto bottom_line = separator() | color(border_col);

    return vbox({bar, bottom_line});
}

int GetTabAtX(int x) {
    for (int i = 0; i < static_cast<int>(g_tab_x_starts.size()); ++i) {
        int next_start = (i + 1 < static_cast<int>(g_tab_x_starts.size()))
            ? g_tab_x_starts[i + 1]
            : 99999;
        if (x >= g_tab_x_starts[i] && x < next_start) {
            return i;
        }
    }
    return -1;
}

}  // namespace FTB::UI
