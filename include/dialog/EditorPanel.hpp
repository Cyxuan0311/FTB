#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace FTB {

struct MainState;

namespace UI {

ftxui::Element RenderEditorPanel(MainState& state, int term_width, int term_height);
bool HandleEditorEvent(MainState& state, const ftxui::Event& event);

}

}
