#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include "core/MainUI.hpp"

namespace FTB { namespace UI {

ftxui::Element RenderOpenerConfigPanel(MainState& state, int tw, int th);
bool HandleOpenerConfigEvent(MainState& state, const ftxui::Event& event);

}} // namespace FTB::UI
