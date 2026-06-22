#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderRenamePanel(MainState& state, int tw, int th);
bool HandleRenameEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
