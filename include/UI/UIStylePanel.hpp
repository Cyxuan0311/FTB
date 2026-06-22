#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include "FTB/MainUI.hpp"

namespace FTB { namespace UI {

ftxui::Element RenderUIStylePanel(MainState& state, int tw, int th);
bool HandleUIStyleEvent(MainState& state, const ftxui::Event& event);

ftxui::Element RenderStatusBarStylePanel(MainState& state, int tw, int th);
bool HandleStatusBarStyleEvent(MainState& state, const ftxui::Event& event);

}} // namespace FTB::UI
