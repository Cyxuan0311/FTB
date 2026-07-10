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
