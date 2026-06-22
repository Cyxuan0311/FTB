#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderCalendarPanel(MainState& state, int tw, int th);
bool HandleCalendarEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
