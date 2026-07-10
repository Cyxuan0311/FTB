#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element RenderAIPanel(MainState& state, int tw, int th, bool fullscreen = false);
bool HandleAIEvent(MainState& state, const ftxui::Event& event);
std::string ExtractAISelectedText(const MainState& state);

} // namespace FTB::UI
