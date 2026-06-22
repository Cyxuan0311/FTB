#include "UI/UIStylePanel.hpp"
#include "FTB/ConfigManager.hpp"
#include "FTB/Powerline.hpp"
#include <algorithm>

namespace FTB { namespace UI {

using namespace ftxui;

namespace {

// ---- 列分隔符字符映射 ----
std::string GetColumnSepChar(const std::string& style) {
    if (style == "light")   return "\u2502";
    if (style == "heavy")   return "\u2503";
    if (style == "double")  return "\u2551";
    if (style == "dotted")  return "\u2506";
    if (style == "dashed")  return "\u250a";
    if (style == "none")    return " ";
    return "";
}

// ---- 列分隔符选项 ----
const std::vector<std::string> kColumnSeparators = {
    "thin", "light", "heavy", "double", "dotted", "dashed", "none"
};

// ---- 面板边框选项 ----
const std::vector<std::string> kPanelBorders = {
    "rounded", "sharp", "double", "heavy", "none"
};

// ---- 选中栏风格选项 ----
const std::vector<std::string> kSelectionStyles = {
    "full", "underline", "invert", "bar", "minimal", "arrow", "rounded"
};

// ---- 状态栏风格选项 ----
const std::vector<std::string> kStatusBarStyles = {
    "powerline", "rounded", "thin", "arrow", "bar", "minimal"
};

int FindIndex(const std::vector<std::string>& list, const std::string& value) {
    auto it = std::find(list.begin(), list.end(), value);
    if (it != list.end()) return static_cast<int>(it - list.begin());
    return 0;
}

// ---- 列分隔符预览（高对比度版） ----
Element ColumnSepPreview(const std::string& style) {
    Color sep_fg = Color::White;
    Color sep_bg = TC("selection_bg");

    auto make_sep = [&]() -> Element {
        if (style == "thin") {
            return separator() | color(sep_fg) | bgcolor(sep_bg);
        }
        std::string ch = GetColumnSepChar(style);
        if (ch == " ") {
            return text(" ") | bgcolor(sep_bg);
        }
        return text(" " + ch + " ") | color(sep_fg) | bgcolor(sep_bg) | bold;
    };
    auto make_name = [&](const std::string& name) -> Element {
        return text(name) | color(TC("main_fg")) | flex;
    };

    Elements parts = {
        make_name("readme.txt"),
        make_sep(),
        make_name("main.cpp"),
        make_sep(),
        make_name("header.h"),
    };

    return hbox(parts) | size(WIDTH, EQUAL, 38);
}

// ---- 面板边框预览（模拟完整面板） ----
Element PanelBorderPreview(const std::string& style) {
    Element content = vbox({
        hbox({
            text(" title ") | bold | color(TC("title")),
            filler()
        }) | bgcolor(TC("main_bg")),
        separator() | color(TC("main_border")),
        text("  item1") | color(TC("main_fg")) | bgcolor(TC("main_bg")),
        text("  item2") | color(TC("main_fg")) | bgcolor(TC("main_bg")),
        text("")
    });

    if (style == "rounded")  return content | borderRounded;
    if (style == "sharp")    return content | border;
    if (style == "double")   return content | borderDouble;
    if (style == "heavy")    return content | borderHeavy;
    if (style == "none")     return content;
    return content | borderRounded;
}

// ---- 状态栏风格预览（高亮分隔符版） ----
Element StatusBarPreview(const std::string& style) {
    Color status_bg = TC("status_bg");
    Color accent_bg = TC("syn_keyword");
    Color main_bg = TC("main_bg");
    const char* sep = GetStatusBarSeparator(style);

    Elements segments;
    segments.push_back(
        text(" NORMAL ") | bold | color(main_bg) | bgcolor(accent_bg)
    );

    if (std::string(sep).empty()) {
        segments.push_back(filler() | bgcolor(status_bg));
        segments.push_back(
            text(" /path ") | color(TC("status_fg")) | bgcolor(status_bg)
        );
        segments.push_back(filler() | bgcolor(status_bg));
    } else {
        segments.push_back(
            text(sep) | color(main_bg) | bgcolor(accent_bg) | bold
        );
        segments.push_back(
            text(" /path ") | color(TC("status_fg")) | bgcolor(status_bg)
        );
        segments.push_back(filler() | bgcolor(status_bg));
    }

    segments.push_back(
        text(" 1/42 ") | color(Color::GrayDark) | bgcolor(status_bg)
    );

    return hbox(segments) | bgcolor(status_bg) | size(WIDTH, EQUAL, 34);
}

// ---- 选中栏风格预览 ----
Element SelectionPreview(const std::string& style) {
    Color sel_bg = TC("selection_bg");
    Color sel_fg = TC("selection_fg");
    Color normal_fg = TC("file");
    Color normal_bg = TC("main_bg");

    auto make_item = [&](const std::string& name, bool sel) -> Element {
        bool shaped = (style == "arrow" || style == "rounded");
        if (sel && shaped) {
            const char* left_ch = (style == "arrow") ? " \ue0b2" : " \ue0b6";
            const char* right_ch = (style == "arrow") ? "\ue0b0 " : "\ue0b4 ";
            return hbox({
                text(left_ch) | color(sel_bg) | bgcolor(normal_bg),
                text(" ") | bgcolor(sel_bg),
                text(name) | color(sel_fg) | bgcolor(sel_bg) | bold | flex,
                text(right_ch) | color(sel_bg) | bgcolor(normal_bg),
            });
        }

        Decorator dec = nothing;
        if (sel) {
            if (style == "full") {
                dec = bgcolor(sel_bg) | color(sel_fg) | bold;
            } else if (style == "underline") {
                dec = color(sel_fg) | bold | underlined;
            } else if (style == "invert") {
                dec = [](Element e) { return e | inverted | bold; };
            } else if (style == "bar") {
                dec = bgcolor(sel_bg);
            } else if (style == "minimal") {
                dec = bold;
            } else {
                dec = bgcolor(sel_bg) | color(sel_fg) | bold;
            }
        }
        bool full_highlight = (style == "full" || style == "bar");
        Color fg = sel ? (full_highlight ? sel_fg : normal_fg) : normal_fg;
        return hbox({
            text(sel ? " > " : "   ") | color(TC("indicator")),
            text(name) | color(fg)
        }) | dec | bgcolor(sel && style == "invert" ? sel_bg : normal_bg);
    };

    return vbox({
        make_item("readme.txt", false),
        make_item("main.cpp", true),
        make_item("header.h", false),
    }) | bgcolor(normal_bg) | size(WIDTH, EQUAL, 28);
}

} // anonymous namespace

// ==================== UI Style Panel ====================

Element RenderUIStylePanel(MainState& state, int /*tw*/, int /*th*/) {
    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();

    int sep_idx = FindIndex(kColumnSeparators, cfg.ui.column_separator);
    int border_idx = FindIndex(kPanelBorders, cfg.ui.panel_border);
    int sel_idx = FindIndex(kSelectionStyles, cfg.ui.selection_style);

    std::string current_sep = kColumnSeparators[sep_idx];
    std::string current_border = kPanelBorders[border_idx];
    std::string current_sel = kSelectionStyles[sel_idx];

    Elements rows;

    // Title
    rows.push_back(
        hbox({
            text(" UI Style") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    // Column Separator section
    rows.push_back(
        text("  Column Separator") | color(TC("path")) | bold | underlined
    );
    {
        std::string indicator = (state.panel_selected == 0) ? " > " : "   ";
        rows.push_back(
            hbox({
                text(indicator) | color(TC("indicator")),
                text(current_sep) | color(TC("main_fg")) | bold,
                text("  "),
                ColumnSepPreview(current_sep)
            })
        );
    }
    rows.push_back(text(""));

    // Panel Border section
    rows.push_back(
        text("  Panel Border") | color(TC("path")) | bold | underlined
    );
    {
        std::string indicator = (state.panel_selected == 1) ? " > " : "   ";
        rows.push_back(
            hbox({
                text(indicator) | color(TC("indicator")),
                text(current_border) | color(TC("main_fg")) | bold,
                text("  "),
                PanelBorderPreview(current_border)
            })
        );
    }
    rows.push_back(text(""));

    // Selection Style section
    rows.push_back(
        text("  Selection Style") | color(TC("path")) | bold | underlined
    );
    {
        std::string indicator = (state.panel_selected == 2) ? " > " : "   ";
        rows.push_back(
            hbox({
                text(indicator) | color(TC("indicator")),
                text(current_sel) | color(TC("main_fg")) | bold,
                text("  "),
                SelectionPreview(current_sel)
            })
        );
    }

    rows.push_back(separator() | color(TC("main_border")));

    // Legend
    rows.push_back(
        hbox({
            text("  ") | color(TC("dim")),
            text("\u2191\u2193 Navigate  ") | color(TC("dim")),
            text("\u2190\u2192 Cycle  ") | color(TC("dim")),
            text("Enter Save  ") | color(TC("syn_keyword")) | bold,
            text("Esc Cancel") | color(TC("dim")),
            filler()
        })
    );

    Element panel = vbox(std::move(rows))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, GREATER_THAN, 50);

    return panel | center;
}

bool HandleUIStyleEvent(MainState& state, const Event& event) {
    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();

    if (event == Event::Escape) {
        cfg.ui.column_separator = state.panel_orig_column_sep;
        cfg.ui.panel_border = state.panel_orig_panel_border;
        cfg.ui.selection_style = state.panel_orig_selection_style;
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Return || event == Event::Character(' ')) {
        ConfigManager::GetInstance()->SaveConfig();
        state.active_panel = ActivePanel::None;
        return true;
    }

    // Navigate between sections
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) state.panel_selected--;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < 2) state.panel_selected++;
        return true;
    }

    // Cycle values with Left/Right
    if (event == Event::ArrowRight || event == Event::Character('l')) {
        if (state.panel_selected == 0) {
            int idx = FindIndex(kColumnSeparators, cfg.ui.column_separator);
            idx = (idx + 1) % kColumnSeparators.size();
            cfg.ui.column_separator = kColumnSeparators[idx];
        } else if (state.panel_selected == 1) {
            int idx = FindIndex(kPanelBorders, cfg.ui.panel_border);
            idx = (idx + 1) % kPanelBorders.size();
            cfg.ui.panel_border = kPanelBorders[idx];
        } else if (state.panel_selected == 2) {
            int idx = FindIndex(kSelectionStyles, cfg.ui.selection_style);
            idx = (idx + 1) % kSelectionStyles.size();
            cfg.ui.selection_style = kSelectionStyles[idx];
        }
        return true;
    }
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        if (state.panel_selected == 0) {
            int idx = FindIndex(kColumnSeparators, cfg.ui.column_separator);
            idx = (idx - 1 + kColumnSeparators.size()) % kColumnSeparators.size();
            cfg.ui.column_separator = kColumnSeparators[idx];
        } else if (state.panel_selected == 1) {
            int idx = FindIndex(kPanelBorders, cfg.ui.panel_border);
            idx = (idx - 1 + kPanelBorders.size()) % kPanelBorders.size();
            cfg.ui.panel_border = kPanelBorders[idx];
        } else if (state.panel_selected == 2) {
            int idx = FindIndex(kSelectionStyles, cfg.ui.selection_style);
            idx = (idx - 1 + kSelectionStyles.size()) % kSelectionStyles.size();
            cfg.ui.selection_style = kSelectionStyles[idx];
        }
        return true;
    }

    return false;
}

// ==================== Status Bar Style Panel ====================

Element RenderStatusBarStylePanel(MainState& state, int /*tw*/, int /*th*/) {
    Elements rows;

    // Title
    rows.push_back(
        hbox({
            text(" Status Bar Style") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    // Style list
    for (int i = 0; i < static_cast<int>(kStatusBarStyles.size()); ++i) {
        bool selected = (state.panel_selected == i);
        std::string indicator = selected ? " > " : "   ";
        Decorator row_style = selected
            ? bgcolor(TC("selection_bg")) | color(TC("selection_fg"))
            : bgcolor(TC("main_bg")) | color(TC("main_fg"));

        rows.push_back(
            hbox({
                text(indicator) | color(TC("indicator")),
                text(kStatusBarStyles[i]) | (selected ? bold : nothing) | size(WIDTH, EQUAL, 12),
                text("  "),
                StatusBarPreview(kStatusBarStyles[i])
            }) | row_style
        );
    }

    rows.push_back(separator() | color(TC("main_border")));

    // Legend
    rows.push_back(
        hbox({
            text("  \u2191\u2193 Select  ") | color(TC("dim")),
            text("Enter Save  ") | color(TC("syn_keyword")) | bold,
            text("Esc Cancel") | color(TC("dim")),
            filler()
        })
    );

    Element panel = vbox(std::move(rows))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, GREATER_THAN, 52);

    return panel | center;
}

bool HandleStatusBarStyleEvent(MainState& state, const Event& event) {
    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();

    if (event == Event::Escape) {
        cfg.statusbar.style = state.panel_orig_statusbar_style;
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Return || event == Event::Character(' ')) {
        ConfigManager::GetInstance()->SaveConfig();
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) {
            state.panel_selected--;
        }
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < static_cast<int>(kStatusBarStyles.size()) - 1) {
            state.panel_selected++;
        }
        return true;
    }

    // Update live preview
    cfg.statusbar.style = kStatusBarStyles[state.panel_selected];
    return true;
}

}} // namespace FTB::UI
