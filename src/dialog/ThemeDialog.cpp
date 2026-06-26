#include "../../include/dialog/ThemeDialog.hpp"
#include "../../include/config/ThemeManager.hpp"
#include "../../include/config/ConfigManager.hpp"

#include <algorithm>

namespace FTB::UI {

using namespace ftxui;

static std::vector<std::string> GetFilteredThemes(const std::string& filter) {
    auto all = FTB::ThemeManager::GetInstance()->GetAvailableThemes();
    if (filter.empty()) return all;

    std::string lower_filter = filter;
    std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);

    std::vector<std::string> result;
    for (const auto& name : all) {
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_filter) != std::string::npos) {
            result.push_back(name);
        }
    }
    return result;
}

Element RenderThemePanel(MainState& state, int tw, int th) {
    int pw = std::min(65, tw - 4);
    int ph = std::min(24, th - 4);
    auto themes = GetFilteredThemes(state.panel_input);
    std::string current_theme = FTB::ThemeManager::GetInstance()->GetCurrentTheme();

    // Clamp selection to valid range
    if (!themes.empty() && state.panel_selected >= static_cast<int>(themes.size())) {
        state.panel_selected = static_cast<int>(themes.size()) - 1;
    }

    // Visible theme rows = panel height - header(4) - footer(1)
    int visible_rows = std::max(3, ph - 5);

    // Clamp scroll so selection is always visible
    if (state.panel_selected < state.theme_scroll)
        state.theme_scroll = state.panel_selected;
    if (state.panel_selected >= state.theme_scroll + visible_rows)
        state.theme_scroll = state.panel_selected - visible_rows + 1;

    // 左侧: 搜索栏 + 主题列表
    Elements list_els;
    list_els.push_back(text(" Themes") | color(TC("title")) | bold);

    // 搜索栏
    std::string search_text = state.panel_input.empty()
        ? " Search: \u258f"
        : " Search: " + state.panel_input + "\u258f";
    list_els.push_back(
        text(search_text) | color(TC("find_keyword"))
    );

    list_els.push_back(separator() | color(TC("main_border")));

    if (themes.empty()) {
        list_els.push_back(text(" (no matches)") | color(TC("dim")) | dim);
    }

    int end = std::min(state.theme_scroll + visible_rows, static_cast<int>(themes.size()));
    for (int i = state.theme_scroll; i < end; ++i) {
        bool is_current = (themes[i] == current_theme);
        bool is_selected = (i == state.panel_selected);
        std::string prefix = is_selected ? " > " : "   ";
        std::string suffix = is_current ? " *" : "";
        auto style = is_selected ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
                                 : color(TC("main_fg"));
        list_els.push_back(hbox({
            text(prefix) | color(TC("indicator")),
            text(themes[i] + suffix) | style
        }));
    }

    // Fill remaining rows so height stays fixed
    int rendered = end - state.theme_scroll;
    int fill_target = themes.empty() ? visible_rows + 1 : visible_rows + 2;
    for (int i = rendered; i < fill_target; ++i)
        list_els.push_back(text(""));

    list_els.push_back(text(" Enter=Apply  Esc=Cancel") | color(TC("dim")) | dim);

    // 右侧: 颜色预览
    Elements preview_els;
    preview_els.push_back(text(" Color Preview") | color(TC("title")) | bold);
    preview_els.push_back(separator() | color(TC("main_border")));

    std::string preview_theme = (!themes.empty() && state.panel_selected >= 0 && state.panel_selected < static_cast<int>(themes.size()))
        ? themes[state.panel_selected] : current_theme;

    std::string saved_theme = FTB::ThemeManager::GetInstance()->GetCurrentTheme();
    if (preview_theme != saved_theme) {
        FTB::ThemeManager::GetInstance()->ApplyTheme(preview_theme);
    }

    std::vector<std::pair<std::string, std::string>> color_items = {
        {"Background", "main_bg"}, {"Foreground", "main_fg"},
        {"Border", "main_border"}, {"Selection", "selection_bg"},
        {"Directory", "directory"}, {"File", "file"},
        {"Executable", "executable"}, {"Link", "link"},
        {"Status BG", "status_bg"}, {"Accent", "find_keyword"},
    };

    for (const auto& [label, color_key] : color_items) {
        Color c = TC(color_key);
        preview_els.push_back(hbox({
            text("  "),
            text("████") | color(c) | bold,
            text(" " + label) | color(TC("main_fg")),
        }));
    }

    if (preview_theme != saved_theme) {
        FTB::ThemeManager::GetInstance()->ApplyTheme(saved_theme);
    }

    return hbox({
        vbox(list_els) | size(WIDTH, EQUAL, 28),
        separator() | color(TC("main_border")),
        vbox(preview_els) | flex,
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleThemeEvent(MainState& state, const Event& event) {
    // 搜索输入
    if (event.is_character()) {
        state.panel_input += event.character();
        state.panel_selected = 0;
        state.theme_scroll = 0;
        return true;
    }

    if (event == Event::Backspace) {
        if (!state.panel_input.empty()) {
            state.panel_input.pop_back();
            state.panel_selected = 0;
            state.theme_scroll = 0;
        }
        return true;
    }

    auto themes = GetFilteredThemes(state.panel_input);
    if (themes.empty()) return true;

    if (event == Event::ArrowUp && state.panel_selected > 0) {
        state.panel_selected--;
        FTB::ThemeManager::GetInstance()->ApplyTheme(themes[state.panel_selected]);
        return true;
    }

    if (event == Event::ArrowDown && state.panel_selected < static_cast<int>(themes.size()) - 1) {
        state.panel_selected++;
        FTB::ThemeManager::GetInstance()->ApplyTheme(themes[state.panel_selected]);
        return true;
    }

    if (event == Event::Return) {
        auto selected_theme = themes[state.panel_selected];
        FTB::ConfigManager::GetInstance()->ApplyTheme(selected_theme);
        FTB::ConfigManager::GetInstance()->SaveConfig();
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }

    return true;
}

}  // namespace FTB::UI
