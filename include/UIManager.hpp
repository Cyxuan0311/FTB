#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <string>
#include <vector>
#include <stack>
#include <atomic>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace UIManager {

    // 处理按键事件及目录操作
    bool handleEvents(ftxui::Event event, 
                      std::stack<std::string>& pathHistory,
                      std::string& currentPath, 
                      std::vector<std::string>& allContents,
                      std::vector<std::string>& filteredContents,
                      int& selected,
                      std::string& searchQuery,
                      ftxui::ScreenInteractive& screen,
                      std::atomic<bool>& refresh_ui);

    // 显示新建文件对话框，并返回新文件名（包含扩展名）
    std::string showNewFileDialog(ftxui::ScreenInteractive& screen);

    // 初始化 UI，返回组合组件及搜索输入组件
    std::pair<ftxui::Component, ftxui::Component> initializeUI(
        std::string& searchQuery,
        std::vector<std::string>& filteredContents,
        int& selected,
        std::stack<std::string>& pathHistory,
        std::string& currentPath,
        std::vector<std::string>& allContents);
}

#endif
