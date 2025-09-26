#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <string>
#include <vector>
#include <atomic>
#include <utility>
#include <memory>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

// 引入 DirectoryHistory、VimLikeEditor 的声明（假设这些头文件位于 FTB 目录下）
#include "FTB/DirectoryHistory.hpp"
#include "FTB/Vim/Vim_Like.hpp"
#include "FTB/FileManager.hpp"

namespace UIManager {

  /**
   * 处理界面事件，并调用相应的事件处理子函数。
   *
   * @param event 当前事件对象。
   * @param directoryHistory 目录历史记录对象。
   * @param currentPath 当前目录路径。
   * @param allContents 当前目录的所有内容。
   * @param filteredContents 根据搜索条件过滤后的内容列表。
   * @param selected 当前选中项的索引。
   * @param searchQuery 当前搜索框中的查询字符串。
   * @param screen ftxui 的 ScreenInteractive 对象，用于界面渲染。
   * @param refresh_ui 原子变量，控制界面刷新。
   * @param vim_mode_active 标识是否进入 Vim 编辑模式。
   * @param vimEditor 指向 Vim 编辑器对象的指针。
   *
   * @return 如果事件被处理返回 true，否则返回 false。
   */
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
                    std::unique_ptr<VimLikeEditor>& vimEditor);

  /**
   * 初始化 UI 组件，返回一个 pair，第一个元素为主容器组件，第二个为搜索输入框组件。
   *
   * @param searchQuery 搜索框的初始查询字符串。
   * @param filteredContents 用于显示的目录内容列表（经过过滤后的）。
   * @param selected 当前选中项的索引。
   * @param directoryHistory 目录历史记录对象。
   * @param currentPath 当前目录路径。
   * @param allContents 当前目录的所有内容。
   *
   * @return 包含主容器组件和搜索输入框组件的 pair。
   */
  std::pair<ftxui::Component, ftxui::Component> initializeUI(std::string& searchQuery,
                                                               std::vector<std::string>& filteredContents,
                                                               int& selected,
                                                               DirectoryHistory& directoryHistory,
                                                               std::string& currentPath,
                                                               std::vector<std::string>& allContents);
}

#endif // UI_MANAGER_HPP
