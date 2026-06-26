#include "core/MainUI.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>

#include "config/ConfigManager.hpp"
#include "browser/FileManager.hpp"
#include "renderer/TextSelection.hpp"

namespace FTB {

using namespace ftxui;

Color GetEntryColor(const FileManager::DirEntryInfo& info) {
    if (info.is_dir) return TC("directory");
    if (info.is_symlink) return TC("link");
    if (info.is_executable) return TC("executable");
    return TC("file");
}

Element BuildFileItem(MainState&, int, bool is_selected, bool is_hovered,
                      const std::string& name, const FileManager::DirEntryInfo& info,
                      const std::string& search_q, bool is_batch_selected) {
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

Element BuildCurrentColumn(MainState& state) {
    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    bool show_hidden = cfg.style.show_hidden_files;

    auto rebuild_filtered = [&]() {
        state.filteredContents.clear();
        for (const auto& item : state.allContents) {
            if (!show_hidden && !item.empty() && item[0] == '.')
                continue;
            if (state.searchQuery.empty() || item.find(state.searchQuery) != std::string::npos)
                state.filteredContents.push_back(item);
        }
    };

    if (!state.searchQuery.empty()) {
        rebuild_filtered();
        state.current_page = 0;
    } else {
        rebuild_filtered();
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
        const std::string& name = state.filteredContents[i];
        FileManager::DirEntryInfo default_info;
        const FileManager::DirEntryInfo* found = nullptr;
        for (const auto& e : state.cached_current_entries) {
            if (e.name == name) { found = &e; break; }
        }
        const FileManager::DirEntryInfo& info = found ? *found : default_info;

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

}
