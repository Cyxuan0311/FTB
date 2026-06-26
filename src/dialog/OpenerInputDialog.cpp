#include "dialog/OpenerInputDialog.hpp"
#include "ops/OpenerManager.hpp"

namespace FTB { namespace UI {

using namespace ftxui;

Element RenderOpenerInputPanel(MainState& state, int /*tw*/, int /*th*/) {
    Elements rows;

    rows.push_back(
        hbox({
            text(" Open With") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    std::string tui_marker = (state.opener_input_mode == 0) ? " > " : "   ";
    std::string gui_marker = (state.opener_input_mode == 1) ? " > " : "   ";

    rows.push_back(
        hbox({
            text("  Mode: ") | color(TC("dim")),
            text(tui_marker) | color(TC("indicator")),
            text("TUI (Block)") | (state.opener_input_mode == 0 ? color(TC("syn_keyword")) | bold : color(TC("main_fg"))),
            text("   "),
            text(gui_marker) | color(TC("indicator")),
            text("GUI (Background)") | (state.opener_input_mode == 1 ? color(TC("syn_keyword")) | bold : color(TC("main_fg"))),
        })
    );
    rows.push_back(text("  Tab to switch mode") | color(TC("dim")));
    rows.push_back(text(""));

    rows.push_back(text("  Command:"));
    rows.push_back(
        hbox({
            text("  > ") | color(TC("syn_keyword")),
            text(state.panel_input) | color(TC("main_fg")),
            text("_") | color(TC("syn_keyword")),
        })
    );

    rows.push_back(text(""));
    rows.push_back(text("  Examples:") | color(TC("dim")));
    rows.push_back(text("    pnana        -> pnana <file> (TUI)") | color(TC("dim")));
    rows.push_back(text("    code         -> code <file> (GUI)") | color(TC("dim")));
    rows.push_back(text("    vim /tmp/a   -> vim /tmp/a (full cmd)") | color(TC("dim")));

    auto names = OpenerManager::Instance().GetOpenerNames();
    if (!names.empty()) {
        rows.push_back(text(""));
        rows.push_back(text("  Registered openers:") | color(TC("dim")));
        std::string list;
        for (size_t i = 0; i < names.size(); ++i) {
            if (i > 0) list += ", ";
            list += names[i];
        }
        rows.push_back(text("  " + list) | color(TC("dim")));
    }

    rows.push_back(separator() | color(TC("main_border")));
    rows.push_back(
        hbox({
            text("  ") | color(TC("dim")),
            text("Tab Mode  ") | color(TC("syn_keyword")) | bold,
            text("Enter Execute  ") | color(TC("syn_keyword")) | bold,
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

bool HandleOpenerInputEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.opener_input_mode = 0;
        return true;
    }

    if (event == Event::Tab) {
        state.opener_input_mode = 1 - state.opener_input_mode;
        return true;
    }

    if (event == Event::Return) {
        std::string input = state.panel_input;
        bool use_block_mode = (state.opener_input_mode == 0);

        if (!input.empty()) {
            bool has_space = (input.find(' ') != std::string::npos);
            std::string cmd_name = has_space ? input.substr(0, input.find(' ')) : input;

            auto opener = OpenerManager::Instance().GetOpener(cmd_name);
            if (opener && !has_space) {
                opener->block = use_block_mode;
                opener->orphan = !use_block_mode;
            } else {
                std::string cmd = has_space ? input : input + " %s";
                Opener custom;
                custom.run = cmd;
                custom.block = use_block_mode;
                custom.orphan = !use_block_mode;
                custom.desc = input;
                opener = custom;
            }

            if (opener) {
                std::string filepath;
                if (state.selected >= 0 &&
                    state.selected < static_cast<int>(state.filteredContents.size())) {
                    filepath = (std::filesystem::path(state.currentPath) / state.filteredContents[state.selected]).string();
                }
                if (!filepath.empty()) {
                    OpenerManager::Instance().Execute(*opener, filepath, state.screen);
                }
            }
        }

        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.opener_input_mode = 0;
        return true;
    }

    if (event == Event::Backspace) {
        if (!state.panel_input.empty()) {
            state.panel_input.pop_back();
        }
        return true;
    }

    if (event.is_character()) {
        char c = event.character()[0];
        if (c != ':') {
            state.panel_input += c;
        }
        return true;
    }

    return true;
}

}} // namespace FTB::UI
