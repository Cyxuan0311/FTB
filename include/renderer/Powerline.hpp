#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>

namespace FTB {

// ---- Powerline 字符 ----
namespace PowerlineChars {
    // 分隔符
    constexpr const char* RightArrow      = "\ue0b0";  //  
    constexpr const char* LeftArrow       = "\ue0b2";  //  
    constexpr const char* RightArrowThin  = "\ue0b1";  //  
    constexpr const char* LeftArrowThin   = "\ue0b3";  //  

    // 圆角分隔符
    constexpr const char* RightRound      = "\ue0b4";  // 
    constexpr const char* LeftRound       = "\ue0b6";  // 
    constexpr const char* RightRoundThin  = "\ue0b5";  // 
    constexpr const char* LeftRoundThin   = "\ue0b7";  // 

    // 其他装饰
    constexpr const char* Line           = "\ue0b1";  //  垂直线
    constexpr const char* Branch         = "\ue0a0";  //  git branch
    constexpr const char* Lock           = "\ue0a2";  // 
    constexpr const char* Dot            = "\ue0b8";  // 
}

// ---- 构建 Powerline 段落 ----
// 创建一个带背景色的段落，右侧带 Powerline 箭头分隔符
ftxui::Element PowerlineSegment(
    const std::string& content,
    ftxui::Color bg_color,
    ftxui::Color fg_color,
    ftxui::Color next_bg_color,
    bool bold = false,
    const std::string& separator = PowerlineChars::RightArrow
);

// ---- 构建 Powerline 标题栏 ----
// 用于列标题，带 Powerline 风格的分隔
ftxui::Element PowerlineHeader(
    const std::string& title,
    ftxui::Color bg_color,
    ftxui::Color fg_color,
    int width = 0
);

// ---- 构建 Powerline 状态栏 ----
// 多段落状态栏
ftxui::Element PowerlineStatusBar(
    ftxui::Elements segments
);

// ---- 构建 Powerline 左侧段落 (右箭头) ----
ftxui::Element PowerlineSegmentLeft(
    const std::string& content,
    ftxui::Color bg_color,
    ftxui::Color fg_color,
    ftxui::Color next_bg_color,
    bool bold = false,
    const std::string& separator = PowerlineChars::RightArrow
);

// ---- 构建 Powerline 右侧段落 (左箭头) ----
ftxui::Element PowerlineSegmentRight(
    const std::string& content,
    ftxui::Color bg_color,
    ftxui::Color fg_color,
    ftxui::Color prev_bg_color,
    bool bold = false,
    const std::string& separator = PowerlineChars::LeftArrow
);

// ---- 根据风格名称获取状态栏分隔符字符 ----
// style: powerline | rounded | thin | arrow | bar | minimal
const char* GetStatusBarSeparator(const std::string& style);

}  // namespace FTB
