#include "UI/SortDialog.hpp"
#include "FTB/ConfigManager.hpp"
#include "FTB/SortMode.hpp"
#include "FTB/FileManager.hpp"
#include <algorithm>

namespace FTB { namespace UI {

using namespace ftxui;

Element RenderSortPanel(MainState& state, int /*tw*/, int /*th*/) {
    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();
    auto current_mode = SortModeFromString(cfg.style.sort_mode);
    auto all_modes = GetAllSortModes();

    Elements rows;

    // Title
    rows.push_back(
        hbox({
            text(" Sort Mode") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    // Sort mode list
    for (int i = 0; i < static_cast<int>(all_modes.size()); ++i) {
        bool selected = (state.panel_selected == i);
        bool is_current = (all_modes[i] == current_mode);

        std::string indicator = selected ? " > " : "   ";
        std::string mode_str = SortModeToString(all_modes[i]);
        std::string desc = SortModeDescription(all_modes[i]);

        Decorator row_style = selected
            ? bgcolor(TC("selection_bg")) | color(TC("selection_fg"))
            : bgcolor(TC("main_bg")) | color(TC("main_fg"));

        auto name_text = text(mode_str) | (selected ? bold : nothing) | size(WIDTH, EQUAL, 14);
        auto desc_text = text(desc) | (selected ? bold : nothing);
        auto mark = is_current && !selected
            ? text(" \u2713") | color(TC("syn_keyword")) | bold
            : text("  ");

        rows.push_back(
            hbox({
                text(indicator) | color(TC("indicator")),
                name_text,
                text("  "),
                desc_text,
                mark,
            }) | row_style
        );
    }

    rows.push_back(separator() | color(TC("main_border")));

    // Legend
    rows.push_back(
        hbox({
            text("  ") | color(TC("dim")),
            text("\u2191\u2193 Select  ") | color(TC("dim")),
            text("Enter Save  ") | color(TC("syn_keyword")) | bold,
            text("Esc Cancel") | color(TC("dim")),
            filler()
        })
    );

    Element panel = vbox(std::move(rows))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, GREATER_THAN, 48);

    return panel | center;
}

bool HandleSortEvent(MainState& state, const Event& event) {
    auto& cfg = ConfigManager::GetInstance()->GetConfigMutable();
    auto all_modes = GetAllSortModes();

    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Return || event == Event::Character(' ')) {
        auto new_mode = all_modes[state.panel_selected];
        cfg.style.sort_mode = SortModeToString(new_mode);
        ConfigManager::GetInstance()->SaveConfig();
        FileManager::invalidateEntryCache();
        state.cached_current_path_for_entries.clear();
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
        if (state.panel_selected < static_cast<int>(all_modes.size()) - 1) {
            state.panel_selected++;
        }
        return true;
    }

    // Live preview: update config + refresh as user navigates
    {
        auto new_mode = all_modes[state.panel_selected];
        cfg.style.sort_mode = SortModeToString(new_mode);
        FileManager::invalidateEntryCache();
        state.cached_current_path_for_entries.clear();
    }

    return true;
}

}} // namespace FTB::UI
