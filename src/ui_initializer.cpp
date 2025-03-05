#include "../include/ui_initializer.hpp"
#include "../include/file_browser.hpp"
#include "../include/file_operations.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <stack>

using namespace ftxui;

std::pair<Component, Component> initializeUI(
    std::string& searchQuery, 
    std::vector<std::string>& filteredContents, 
    int& selected,
    std::stack<std::string>& pathHistory,
    std::string& currentPath,
    std::vector<std::string>& allContents) 
{
    // 创建搜索输入组件
    auto searchInput = Input(&searchQuery, "搜索...");

    // 设置菜单选项，定义按下回车时进入目录的行为
    MenuOption menu_option;
    menu_option.on_enter = [&] {
        enterDirectory(pathHistory, currentPath, filteredContents, selected);
        allContents = FileBrowser::getDirectoryContents(currentPath);
        filteredContents = allContents;
        searchQuery.clear();
    };

    // 创建选择器组件，传入菜单选项
    auto selector = Menu(&filteredContents, &selected, menu_option);

    // 为选择器添加鼠标事件处理
    auto mouse_component = CatchEvent(selector, [&](Event event) {
        if (event.is_mouse() && event.mouse().motion == Mouse::Moved) {
            return true;
        }
        if (event.is_mouse() && event.mouse().button == Mouse::Left && 
            event.mouse().motion == Mouse::Pressed) {
            menu_option.on_enter();
            return true;
        }
        return false;
    });

    // 组合搜索输入和鼠标事件处理后的选择器
    auto container = Container::Vertical({searchInput, mouse_component});

    return std::make_pair(container, searchInput);
}