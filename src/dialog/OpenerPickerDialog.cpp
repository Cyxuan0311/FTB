#include "dialog/OpenerPickerDialog.hpp"
#include "ops/OpenerManager.hpp"

namespace FTB { namespace UI {

using namespace ftxui;

Element RenderOpenerPickerPanel(MainState& state, int /*tw*/, int /*th*/) {
    Elements rows;

    rows.push_back(
        hbox({
            text(" Open With") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    if (state.matched_openers.empty()) {
        rows.push_back(text("  No matchers found") | color(TC("dim")));
    } else {
        for (int i = 0; i < static_cast<int>(state.matched_openers.size()); ++i) {
            bool selected = (state.opener_selected == i);
            const auto& [name, opener] = state.matched_openers[i];

            std::string indicator = selected ? " > " : "   ";

            Decorator row_style = selected
                ? bgcolor(TC("selection_bg")) | color(TC("selection_fg"))
                : bgcolor(TC("main_bg")) | color(TC("main_fg"));

            auto name_text = text(name) | (selected ? bold : nothing) | size(WIDTH, EQUAL, 12);
            auto desc_text = text(opener.desc.empty() ? opener.run : opener.desc)
                | (selected ? bold : nothing);

            rows.push_back(
                hbox({
                    text(indicator) | color(TC("indicator")),
                    name_text,
                    text("  "),
                    desc_text,
                }) | row_style
            );
        }
    }

    rows.push_back(separator() | color(TC("main_border")));
    rows.push_back(
        hbox({
            text("  ") | color(TC("dim")),
            text("\u2191\u2193 Select  ") | color(TC("dim")),
            text("Enter Open  ") | color(TC("syn_keyword")) | bold,
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

bool HandleOpenerPickerEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Return) {
        if (!state.matched_openers.empty() &&
            state.opener_selected >= 0 &&
            state.opener_selected < static_cast<int>(state.matched_openers.size())) {
            const auto& [name, opener] = state.matched_openers[state.opener_selected];
            std::string filepath;
            if (state.selected >= 0 &&
                state.selected < static_cast<int>(state.filteredContents.size())) {
                filepath = (std::filesystem::path(state.currentPath) / state.filteredContents[state.selected]).string();
            }
            if (!filepath.empty()) {
                OpenerManager::Instance().Execute(opener, filepath, state.screen);
            }
        }
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.opener_selected > 0) {
            state.opener_selected--;
        }
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.opener_selected < static_cast<int>(state.matched_openers.size()) - 1) {
            state.opener_selected++;
        }
        return true;
    }

    return true;
}

}} // namespace FTB::UI
