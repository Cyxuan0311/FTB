#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace FTB {

struct MainState;

namespace UI {

ftxui::Element RenderImagePreviewPanel(MainState& state, int term_width, int term_height);
bool HandleImagePreviewEvent(MainState& state, ftxui::Event& event);

}

}
