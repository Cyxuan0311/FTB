#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderLayoutPanel(MainState& state, int tw, int th);
bool HandleLayoutEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
