#ifndef DETAIL_ELEMENT_HPP
#define DETAIL_ELEMENT_HPP

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>
#include "ClipboardManager.hpp"
#include "FileManager.hpp"
#include "IconMapper.hpp"

using namespace ftxui;
using namespace FileManager;

Element RenderPendingFiles() {
  auto& clipboard = ClipboardManager::getInstance();
  const auto& items = clipboard.getItems();
  
  Elements pendingElements;
  Elements headerElements;
  headerElements.push_back(text("📋 待处理项目：") | bold | color(Color::BlueLight));
  
  // 只有在设置了模式且有待处理项目时才显示模式状态
  if (!items.empty() && clipboard.hasModeSelected()) {  // 使用新的判断方法
      headerElements.push_back(
          text(clipboard.isCutMode() ?  "✂️ 剪切模式" : "📑 复制模式") |
          (clipboard.isCutMode() ? color(Color::Red) : color(Color::Green3))
      );
  }
  
  pendingElements.push_back(hbox(headerElements) | center);
  
  if (items.empty()) {
      pendingElements.push_back(text("(无)") | dim);
  } else {
      for (const auto& path : items) {
          fs::path p(path);
          auto filename = p.filename().string();
          auto icon = FTB::Icons::GetIconForPath(p, fs::is_directory(p));
          auto fileElement = text(icon + filename);
          
          // 只有在设置了模式后才显示颜色
          if (clipboard.hasModeSelected()) {  // 使用新的判断方法
              if (clipboard.isCutMode()) {
                  fileElement = fileElement | color(Color::Red);
              } else {
                  fileElement = fileElement | color(Color::Green3);
              }
          }
          pendingElements.push_back(fileElement);
      }
  }

  pendingElements.push_back(separator());
  pendingElements.push_back(text("Alt+C: 添加") | dim | color(Color::GrayLight));
  pendingElements.push_back(text("Alt+G: 清空") | dim | color(Color::GrayLight));
  pendingElements.push_back(text("Ctrl+T: 复制") | dim | color(Color::GrayLight));
  pendingElements.push_back(text("Ctrl+X: 剪切") | dim | color(Color::GrayLight));
  pendingElements.push_back(text("Ctrl+N: 粘贴") | dim | color(Color::GrayLight));

  return vbox(std::move(pendingElements)) 
         | borderHeavy 
         | color(Color::RGB(77, 153, 0))
         | flex;
}

// 生成当前月份的日历，返回一个 Element 列表，每个 Element 表示一行
std::vector<Element> GenerateCalendarElements() {
  std::vector<Element> lines;

  // 获取当前时间和今天的日期
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  int today = tm.tm_mday;

  // 标题行：YYYY-MM
  std::ostringstream header;
  header << std::put_time(&tm, "%Y-%m");
  lines.push_back(text(header.str()) | bold);

  // 星期标题行
  lines.push_back(text("Sun Mon Tue Wed Thu Fri Sat") | color(Color::Green3));

  // 将 tm 调整为本月第一天
  tm.tm_mday = 1;
  std::mktime(&tm);
  int first_weekday = tm.tm_wday;

  // 构造当前一周的日期行
  Elements current_week;
  // 填充前面的空白（如果第一天不是星期日）
  for (int i = 0; i < first_weekday; ++i)
    current_week.push_back(text("    "));

  // 遍历 1 到 31 天，构建日期元素
  for (int day = 1; day <= 31; ++day) {
    tm.tm_mday = day;
    // 如果日期无效则退出循环
    if (std::mktime(&tm) == -1) break;

    std::ostringstream day_stream;
    day_stream << std::setw(3) << day;
    Element day_elem = text(day_stream.str());

    // 如果当前天数是今天，则设置特殊颜色
    if (day == today) {
      day_elem = day_elem | color(Color::Blue3Bis) | bold;
    } else {
      day_elem = day_elem | color(Color::Black);
    }
    // 每个日期后添加一个空格
    current_week.push_back(hbox({day_elem, text(" ")}));

    // 当一行满 7 个日期后，放入一行，并清空 current_week
    if ((first_weekday + day) % 7 == 0) {
      lines.push_back(hbox(current_week));
      current_week.clear();
    }
  }
  // 如果最后一行未满 7 个日期，将其加入
  if (!current_week.empty()) {
    lines.push_back(hbox(current_week));
  }
  return lines;
}

// 修改后的 CreateDetailElement，将日历加入详情区域
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

  // 使用统一的 Nerd Font 图标映射
  fs::path selected_path = fs::path(currentPath) / selectedName;
  std::string icon = FTB::Icons::GetIconForPath(selected_path, is_dir);

  // 生成日历的 Element 列表
  auto calendar_elements = GenerateCalendarElements();

  return vbox({
    text("侧边栏") | bold | borderLight | color(Color::SkyBlue2) | center,
    window(text("当前选中") | color(Color::Cyan1),
           text(icon + selectedName) | color(Color::Yellow3) | borderHeavy),
    // 日历区域
    vbox(calendar_elements) | borderDouble | bgcolor(Color::GrayLight) | size(WIDTH, EQUAL, 30) | size(HEIGHT, EQUAL, 10),
    RenderPendingFiles()
  }) | borderHeavy | color(Color::GrayDark) | flex;
}

#endif // DETAIL_ELEMENT_HPP
