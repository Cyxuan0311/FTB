#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderNewFolderPanel(MainState& state, int tw, int th);
bool HandleNewFolderEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
