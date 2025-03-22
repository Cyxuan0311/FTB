#ifndef DETAIL_ELEMENT_HPP
#define DETAIL_ELEMENT_HPP

#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <ftxui/dom/elements.hpp>
#include "../include/FTB/FileManager.hpp"

using namespace ftxui;
using namespace FileManager;

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
  lines.push_back(text("Sun Mon Tue Wed Thu Fri Sat"));

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
      day_elem = day_elem | color(Color::White);
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

  // 使用不同图标：文件夹用 "📁 "，文件用 "📄 "
  std::string icon = is_dir ? "📁 " : "📄 ";

  // 生成日历的 Element 列表
  auto calendar_elements = GenerateCalendarElements();

  return vbox({
    text("侧边栏") | bold | borderLight | color(Color::SkyBlue2) | center,
    window(text("当前选中") | color(Color::Cyan1),
           text(icon + selectedName) | color(Color::Yellow3) | borderHeavy),
    // 日历区域：固定尺寸，边框圆角，背景颜色为 RGB(53,53,115)
    vbox(calendar_elements) | borderDouble | bgcolor(Color::RGB(255, 223, 128)) | size(WIDTH, EQUAL, 30) | size(HEIGHT, EQUAL, 10)

  }) | borderHeavy | color(Color::GrayDark) | flex;
}

#endif // DETAIL_ELEMENT_HPP
