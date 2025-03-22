#ifndef DETAIL_ELEMENT_HPP
#define DETAIL_ELEMENT_HPP

#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>
#include "FileManager.hpp" // 确保包含文件管理相关接口

using namespace ftxui;

// 传入过滤后的内容列表、当前选中的索引以及当前路径，返回详情区域的 Element。
// 根据当前选中的项判断是否为文件夹，并显示不同图标。
inline Element CreateDetailElement(const std::vector<std::string>& filteredContents,
                                   int selected,
                                   const std::string& currentPath) {
  std::string selectedName;
  bool is_dir = false;
  if (selected >= 0 && selected < static_cast<int>(filteredContents.size())) {
    selectedName = filteredContents[selected];
    std::string fullPath = currentPath + "/" + selectedName;
    is_dir = FileManager::isDirectory(fullPath);
  } else {
    selectedName = "无选中项";
  }

  // 使用不同的图标：文件夹使用 "📁 "，文件使用 "📄 "
  std::string icon = is_dir ? "📁 " : "📄 ";

  return vbox({ 
    text("详情信息") | bold | borderLight | color(Color::SkyBlue2) | center,
    // 将标题字符串用 text() 包裹，使其成为 Element
    window(text("当前选中") | color(Color::Cyan1), text(icon + selectedName) | color(Color::Yellow3) | borderHeavy),
    text("这里可以放置更多数据或操作")
  }) | borderHeavy | color(Color::GrayDark) | flex;
}

#endif // DETAIL_ELEMENT_HPP
