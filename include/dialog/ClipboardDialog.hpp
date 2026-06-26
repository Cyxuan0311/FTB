#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderClipboardPanel(MainState& state, int tw, int th);
bool HandleClipboardEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
