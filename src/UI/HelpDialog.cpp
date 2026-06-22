#include "../../include/UI/HelpDialog.hpp"
#include "../../include/FTB/KeyBindings.hpp"

namespace FTB::UI {

using namespace ftxui;

Element RenderHelpPanel(MainState& state, int tw, int th) {
    int pw = std::min(60, tw - 4);
    int ph = std::min(30, th - 4);
    auto& keybindings = FTB::KeyBindings::GetInstance();
    auto commands = keybindings.GetCommandList();

    // Visible content rows = panel height - tab_bar(1) - separator(1) - footer(1)
    int visible_rows = std::max(5, ph - 3);

    Elements tab_bar;
    tab_bar.push_back(text(" "));
    auto tab0 = text(" Keybindings ") | (state.help_tab == 0
        ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
        : color(TC("dim")));
    auto tab1 = text(" Commands ") | (state.help_tab == 1
        ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
        : color(TC("dim")));
    tab_bar.push_back(tab0);
    tab_bar.push_back(tab1);
    tab_bar.push_back(text(" "));

    // Build content based on tab
    Elements all_content;
    if (state.help_tab == 0) {
        all_content.push_back(text(""));
        all_content.push_back(text(" File List") | color(TC("title")) | bold);
        all_content.push_back(text("   j / Down          Move down") | color(TC("main_fg")));
        all_content.push_back(text("   k / Up            Move up") | color(TC("main_fg")));
        all_content.push_back(text("   h / Left          Parent directory") | color(TC("main_fg")));
        all_content.push_back(text("   l / Right         Enter directory") | color(TC("main_fg")));
        all_content.push_back(text("   Home              Jump to top") | color(TC("main_fg")));
        all_content.push_back(text("   End               Jump to bottom") | color(TC("main_fg")));
        all_content.push_back(text("   PageUp / PageDown Scroll page") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+F            Toggle hidden files") | color(TC("main_fg")));
        all_content.push_back(text("   Space             Select item") | color(TC("main_fg")));
        all_content.push_back(text("   Enter             Open selected") | color(TC("main_fg")));
        all_content.push_back(text(""));
        all_content.push_back(text(" Search") | color(TC("title")) | bold);
        all_content.push_back(text("   /                 Start search (type to filter)") | color(TC("main_fg")));
        all_content.push_back(text("   Escape            Clear search") | color(TC("main_fg")));
        all_content.push_back(text("   n / N             Next / Prev match") | color(TC("main_fg")));
        all_content.push_back(text(""));
        all_content.push_back(text(" Commands (Ctrl+B)") | color(TC("title")) | bold);
        all_content.push_back(text("   Ctrl+B            Enter :command prefix mode") | color(TC("main_fg")));
        all_content.push_back(text("   Tab               Complete command") | color(TC("main_fg")));
        all_content.push_back(text("   Enter             Execute command") | color(TC("main_fg")));
        all_content.push_back(text("   Escape            Cancel command") | color(TC("main_fg")));
        all_content.push_back(text(""));
        all_content.push_back(text(" Editor (Ctrl+B :e/:v)") | color(TC("title")) | bold);
        all_content.push_back(text("   Ctrl+O            Save file") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+X            Exit editor") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+K            Cut current line") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+U            Paste at cursor") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+W            Search text") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+V            PageDown") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+Y            PageUp") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+Z            Undo") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+_            Go to line") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+C            Cursor position") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+A / Ctrl+E   Line start / end") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+B / Ctrl+F   Char left / right") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+P / Ctrl+N   Line up / down") | color(TC("main_fg")));
        all_content.push_back(text("   Alt+M             Toggle Markdown preview") | color(TC("main_fg")));
        all_content.push_back(text("   Alt+U / Alt+E     Undo / Redo") | color(TC("main_fg")));
        all_content.push_back(text(""));
        all_content.push_back(text(" Other") | color(TC("title")) | bold);
        all_content.push_back(text("   Ctrl+R            Reload config") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+C (outside)  Quit FTB") | color(TC("main_fg")));
        all_content.push_back(text("   Ctrl+O            Open file preview") | color(TC("main_fg")));
    } else {
        all_content.push_back(text(""));
        all_content.push_back(text(" Type :command after Ctrl+B, e.g. Ctrl+B th <Enter>") | color(TC("dim")) | dim);
        all_content.push_back(text(""));
        for (const auto& [cmd, desc] : commands) {
            all_content.push_back(hbox({
                text("   :" + cmd) | color(TC("find_keyword")) | bold,
                text(std::string(std::max(1, 16 - static_cast<int>(cmd.size())), ' ')),
                text(desc) | color(TC("main_fg")),
            }));
        }
    }
    all_content.push_back(text(""));
    all_content.push_back(text(" Tab=Switch  Up/Dn=Scroll  q/Esc=Close") | color(TC("dim")) | dim);

    // Clamp scroll so bottom of content is visible
    int total = static_cast<int>(all_content.size());
    int max_scroll = std::max(0, total - visible_rows);
    if (state.help_scroll > max_scroll) state.help_scroll = max_scroll;

    // Slice visible portion
    Elements visible;
    int slice_end = std::min(state.help_scroll + visible_rows, total);
    for (int i = state.help_scroll; i < slice_end; ++i)
        visible.push_back(all_content[i]);

    // Fill remaining rows so height stays fixed
    for (int i = visible.size(); i < visible_rows; ++i)
        visible.push_back(text(""));

    return vbox({
        hbox(tab_bar),
        separator() | color(TC("main_border")),
        vbox(visible) | flex,
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleHelpEvent(MainState& state, const Event& event) {
    auto& keybindings = FTB::KeyBindings::GetInstance();
    auto commands = keybindings.GetCommandList();
    int ph = std::min(30, 200);  // approximate, used for scroll amount
    int visible_rows = std::max(5, ph - 3);

    if (event == Event::Tab) {
        state.help_tab = 1 - state.help_tab;
        state.help_scroll = 0;
        return true;
    }
    if (event == Event::Character('q') || event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }
    if (event == Event::ArrowUp) {
        if (state.help_scroll > 0) state.help_scroll--;
        return true;
    }
    if (event == Event::ArrowDown) {
        state.help_scroll++;
        return true;
    }
    if (event == Event::PageUp || event == Event::CtrlY) {
        state.help_scroll = std::max(0, state.help_scroll - visible_rows);
        return true;
    }
    if (event == Event::PageDown || event == Event::CtrlV) {
        state.help_scroll += visible_rows;
        return true;
    }
    if (event == Event::Home) {
        state.help_scroll = 0;
        return true;
    }
    if (event == Event::End) {
        state.help_scroll = 9999;  // clamped in Render
        return true;
    }
    return true;
}

}  // namespace FTB::UI
