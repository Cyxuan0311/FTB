#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <stack>
#include <string>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <atomic>

bool handleEvents(ftxui::Event event,
                  std::stack<std::string>& pathHistory,
                  std::string& currentPath,
                  std::vector<std::string>& allContents,
                  std::vector<std::string>& filteredContents,
                  int& selected,
                  std::string& searchQuery,
                  ftxui::ScreenInteractive& screen,
                  std::atomic<bool>& refresh_ui);

#endif // EVENT_HANDLER_HPP
