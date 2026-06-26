#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include "core/MainUI.hpp"

namespace FTB { namespace UI {

ftxui::Element RenderSortPanel(MainState& state, int tw, int th);
bool HandleSortEvent(MainState& state, const ftxui::Event& event);

}} // namespace FTB::UI
