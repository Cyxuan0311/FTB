#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <string>
#include <vector>
#include <atomic>
#include "DirectoryHistory.hpp"
#include "Vim_Like.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace UIManager {

bool handleEvents(ftxui::Event event,
                  DirectoryHistory& directoryHistory,
                  std::string& currentPath,
                  std::vector<std::string>& allContents,
                  std::vector<std::string>& filteredContents,
                  int& selected,
                  std::string& searchQuery,
                  ftxui::ScreenInteractive& screen,
                  std::atomic<bool>& refresh_ui,
                  bool& vim_mode_active,
                  VimLikeEditor*& vimEditor);

std::string showNewFileDialog(ftxui::ScreenInteractive& screen);

std::pair<ftxui::Component, ftxui::Component> initializeUI(
    std::string& searchQuery,
    std::vector<std::string>& filteredContents,
    int& selected,
    DirectoryHistory& directoryHistory,
    std::string& currentPath,
    std::vector<std::string>& allContents);

}

#endif // UI_MANAGER_HPP
