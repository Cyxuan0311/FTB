#ifndef UI_INITIALIZER_HPP
#define UI_INITIALIZER_HPP

#include <ftxui/component/component.hpp>
#include <string>
#include <vector>
#include <stack>
#include <utility>

std::pair<ftxui::Component, ftxui::Component> initializeUI(
    std::string& searchQuery, 
    std::vector<std::string>& filteredContents, 
    int& selected,
    std::stack<std::string>& pathHistory,
    std::string& currentPath,
    std::vector<std::string>& allContents);

#endif // UI_INITIALIZER_HPP
