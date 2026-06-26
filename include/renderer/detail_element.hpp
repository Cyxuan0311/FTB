#ifndef DETAIL_ELEMENT_HPP
#define DETAIL_ELEMENT_HPP

#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>

#include "config/ThemeManager.hpp"
#include "config/ConfigManager.hpp"
#include "browser/FileManager.hpp"
#include "preview/PreviewCache.hpp"

namespace FTB {

inline ftxui::Color TC(const std::string& name) {
    return FTB::ThemeManager::GetInstance()->GetThemeColor(name);
}

inline ftxui::Decorator GetPanelBorder() {
    const auto& style = FTB::ConfigManager::GetInstance()->GetConfig().ui.panel_border;
    ftxui::Color border_color = TC("main_border");
    if (style == "rounded")  return ftxui::borderStyled(ftxui::ROUNDED, border_color);
    if (style == "sharp")    return ftxui::borderStyled(ftxui::LIGHT, border_color);
    if (style == "double")   return ftxui::borderStyled(ftxui::DOUBLE, border_color);
    if (style == "heavy")    return ftxui::borderStyled(ftxui::HEAVY, border_color);
    if (style == "none")     return ftxui::nothing;
    return ftxui::borderStyled(ftxui::ROUNDED, border_color);
}

namespace DetailColor {
    const ftxui::Color Base     = ftxui::Color::RGB(0x1e, 0x1e, 0x2e);
    const ftxui::Color Mantle   = ftxui::Color::RGB(0x18, 0x18, 0x25);
    const ftxui::Color Surface0 = ftxui::Color::RGB(0x31, 0x32, 0x44);
    const ftxui::Color Surface1 = ftxui::Color::RGB(0x45, 0x47, 0x5a);
    const ftxui::Color Surface2 = ftxui::Color::RGB(0x58, 0x5b, 0x70);
    const ftxui::Color Overlay0 = ftxui::Color::RGB(0x6c, 0x70, 0x86);
    const ftxui::Color Text     = ftxui::Color::RGB(0xcd, 0xd6, 0xf4);
    const ftxui::Color Subtext  = ftxui::Color::RGB(0xa6, 0xad, 0xbc);
    const ftxui::Color Blue     = ftxui::Color::RGB(0x89, 0xb4, 0xfa);
    const ftxui::Color Green    = ftxui::Color::RGB(0xa6, 0xe3, 0xa1);
    const ftxui::Color Red      = ftxui::Color::RGB(0xf3, 0x8b, 0xa8);
    const ftxui::Color Yellow   = ftxui::Color::RGB(0xf9, 0xe2, 0xaf);
    const ftxui::Color Peach    = ftxui::Color::RGB(0xfa, 0xb3, 0x87);
    const ftxui::Color Mauve    = ftxui::Color::RGB(0xcb, 0xa6, 0xf7);
    const ftxui::Color Pink     = ftxui::Color::RGB(0xf5, 0xc2, 0xe7);
    const ftxui::Color Teal     = ftxui::Color::RGB(0x94, 0xe2, 0xd5);
}

ftxui::Element CreateDetailElement(const std::vector<FileManager::DirEntryInfo>& entries,
                                   int selected,
                                   const std::string& currentPath,
                                   int scroll_y = 0,
                                   int scroll_x = 0);

}

using namespace ftxui;
using namespace FileManager;
using namespace FTB;

#endif // DETAIL_ELEMENT_HPP
