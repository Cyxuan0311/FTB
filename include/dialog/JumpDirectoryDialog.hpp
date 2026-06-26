#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderJumpDirectoryPanel(MainState& state, int tw, int th);
bool HandleJumpDirectoryEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
