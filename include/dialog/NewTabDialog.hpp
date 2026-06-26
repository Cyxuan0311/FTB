#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderNewTabPanel(MainState& state, int tw, int th);
bool HandleNewTabEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
