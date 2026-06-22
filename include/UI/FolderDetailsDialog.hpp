#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderFolderDetailsPanel(MainState& state, int tw, int th);
bool HandleFolderDetailsEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
