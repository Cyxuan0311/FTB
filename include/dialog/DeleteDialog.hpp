#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderDeleteConfirmPanel(MainState& state, int tw, int th);
bool HandleDeleteConfirmEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
