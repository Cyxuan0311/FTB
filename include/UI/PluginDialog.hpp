#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderPluginPanel(MainState& state, int tw, int th);
bool HandlePluginEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
