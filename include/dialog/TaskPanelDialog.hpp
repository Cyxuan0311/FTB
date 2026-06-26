#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderTaskPanel(MainState& state, int tw, int th);
bool HandleTaskPanelEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
