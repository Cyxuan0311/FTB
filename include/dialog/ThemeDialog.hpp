#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderThemePanel(MainState& state, int tw, int th);
bool HandleThemeEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
