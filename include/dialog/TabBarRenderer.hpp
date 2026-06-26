#pragma once
#include "core/MainUI.hpp"
#include <ftxui/dom/elements.hpp>

namespace FTB::UI {

ftxui::Element BuildTabBar(MainState& state);
int GetTabAtX(int x);

}  // namespace FTB::UI
