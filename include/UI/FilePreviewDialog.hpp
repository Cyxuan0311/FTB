#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderFilePreviewPanel(MainState& state, int tw, int th);
bool HandleFilePreviewEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
