#pragma once
#include "FTB/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {
ftxui::Element RenderFuzzyFinderPanel(MainState& state, int tw, int th);
bool HandleFuzzyFinderEvent(MainState& state, const ftxui::Event& event);
}
