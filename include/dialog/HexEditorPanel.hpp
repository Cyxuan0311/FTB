#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace FTB {

struct MainState;

namespace UI {

ftxui::Element RenderHexEditorPanel(MainState& state, int term_width, int term_height);
bool HandleHexEditorEvent(MainState& state, ftxui::Event& event);

}

}
