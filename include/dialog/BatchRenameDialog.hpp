#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderBatchRenamePanel(MainState& state, int tw, int th);
bool HandleBatchRenameEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB::UI
