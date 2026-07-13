#include "dialog/EditorPanel.hpp"
#include "core/MainUI.hpp"
#include "config/ThemeManager.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB {
namespace UI {

using namespace ftxui;

static Color TC(const std::string& name) {
    return ThemeManager::GetInstance()->GetThemeColor(name);
}

Element RenderEditorPanel(MainState& state, int term_width, int term_height) {
    auto& tab = state.tabManager.active();
    if (!tab.editor) return text("No editor");

    auto* editor = tab.editor.get();

    int panel_w = term_width - 2;
    int panel_h = term_height;
    int header_h = 1;
    int status_h = 1;
    int content_h = panel_h - header_h - status_h;
    if (content_h < 3) content_h = 3;
    int content_w = panel_w;

    // Micro-style tab header
    std::string fn = editor->GetFilename();
    if (fn.empty()) fn = "untitled";
    std::string modified = editor->IsModified() ? " *" : "";

    Element header = hbox({
        text(" " + fn + modified + " ") | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        filler() | bgcolor(TC("status_bg")),
        text(" [Esc] Close ") | color(Color::Grey37) | bgcolor(TC("status_bg")),
    }) | size(WIDTH, EQUAL, panel_w);

    // Editor content area
    Element content = editor->RenderContent(content_w, content_h);

    // Micro-style status line
    std::string status_text = editor->GetStatusLine();
    Element status = hbox({
        text(status_text) | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        filler() | bgcolor(TC("status_bg")),
    }) | size(WIDTH, EQUAL, panel_w);

    return vbox({
        header,
        content | flex,
        status,
    }) | bgcolor(TC("main_bg"));
}

bool HandleEditorEvent(MainState& state, const Event& event) {
    auto& tab = state.tabManager.active();
    if (!tab.editor) return false;

    // Escape closes the editor tab
    if (event == Event::Escape) {
        state.saveTabState();
        int idx = state.tabManager.activeIndex();
        if (state.tabManager.isEditorTab(idx) && state.tabManager.canClose()) {
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        // Can't close last tab - create a directory tab first
        if (state.tabManager.isEditorTab(idx) && !state.tabManager.canClose()) {
            const char* home = std::getenv("HOME");
            state.tabManager.createTab(home ? home : "/");
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        return false;
    }

    return tab.editor->OnEvent(event);
}

}
}
