#pragma once
#include "FTB/MainUI.hpp"
#include "Connection/SSHConnection.hpp"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace FTB::UI {

ftxui::Element RenderSSHPanel(FTB::MainState& state, int tw, int th);
bool HandleSSHEvent(FTB::MainState& state, const ftxui::Event& event);
void disconnectSSH(FTB::MainState& state);
Connection::SSHConnection* GetSSHConnection();

}  // namespace FTB::UI
