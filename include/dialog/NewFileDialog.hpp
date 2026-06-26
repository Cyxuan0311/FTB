#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderNewFilePanel(MainState& state, int tw, int th);
bool HandleNewFileEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
