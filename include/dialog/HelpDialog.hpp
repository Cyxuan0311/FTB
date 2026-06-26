#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderHelpPanel(MainState& state, int tw, int th);
bool HandleHelpEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
