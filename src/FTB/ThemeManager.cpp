#include "FTB/ThemeManager.hpp"
#include <iostream>
#include <algorithm>

namespace FTB {

std::unique_ptr<ThemeManager> ThemeManager::instance_ = nullptr;

ThemeManager::ThemeManager() : current_theme_("default") {
    InitializePredefinedThemes();
    ApplyTheme("default");
}

ThemeManager* ThemeManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = std::make_unique<ThemeManager>();
    }
    return instance_.get();
}

void ThemeManager::ApplyTheme(const std::string& theme_name) {
    if (current_theme_ == theme_name) {
        return;
    }
    
    current_theme_ = theme_name;
    ApplyThemeConfig(theme_name);
    CreateThemeColorMap(theme_name);
    
    
}

std::vector<std::string> ThemeManager::GetAvailableThemes() const {
    std::vector<std::string> themes;
    themes.reserve(predefined_themes_.size());  // 预分配空间
    for (const auto& theme : predefined_themes_) {
        themes.push_back(theme.first);
    }
    return themes;
}

ftxui::Color ThemeManager::GetThemeColor(const std::string& color_name) const {
    auto it = current_theme_colors_.find(color_name);
    if (it != current_theme_colors_.end()) {
        return it->second;
    }
    
    // 返回默认颜色
    return ftxui::Color::White;
}

ftxui::Color ThemeManager::GetFileTypeColor(const std::string& file_type) const {
    if (file_type == "directory") return GetThemeColor("directory");
    if (file_type == "executable") return GetThemeColor("executable");
    if (file_type == "link") return GetThemeColor("link");
    if (file_type == "hidden") return GetThemeColor("hidden");
    if (file_type == "system") return GetThemeColor("system");
    
    return GetThemeColor("file");
}

ftxui::Element ThemeManager::ApplyColorToElement(ftxui::Element element, const std::string& color_name) const {
    return ftxui::color(GetThemeColor(color_name), element);
}

ftxui::Element ThemeManager::CreateColoredText(const std::string& text, const std::string& color_name) const {
    return ftxui::color(GetThemeColor(color_name), ftxui::text(text));
}

ftxui::Element ThemeManager::CreateColoredBorder(ftxui::Element element, const std::string& color_name) const {
    return ftxui::borderRounded(element) | ftxui::color(GetThemeColor(color_name));
}

ftxui::Element ThemeManager::CreateColoredBackground(ftxui::Element element, const std::string& color_name) const {
    return ftxui::bgcolor(GetThemeColor(color_name), element);
}

ftxui::Element ThemeManager::CreateSelectionStyle(ftxui::Element element) const {
    return ftxui::bgcolor(GetThemeColor("selection_bg"), 
           ftxui::color(GetThemeColor("selection_fg"), element));
}

ftxui::Element ThemeManager::CreateStatusBarStyle(ftxui::Element element) const {
    return ftxui::bgcolor(GetThemeColor("status_bg"), 
           ftxui::color(GetThemeColor("status_fg"), element));
}

ftxui::Element ThemeManager::CreateSearchBoxStyle(ftxui::Element element) const {
    return ftxui::borderRounded(
           ftxui::bgcolor(GetThemeColor("search_bg"), 
           ftxui::color(GetThemeColor("search_fg"), element))) | 
           ftxui::color(GetThemeColor("search_border"));
}

ftxui::Element ThemeManager::CreateDialogStyle(ftxui::Element element) const {
    return ftxui::borderRounded(
           ftxui::bgcolor(GetThemeColor("dialog_bg"), 
           ftxui::color(GetThemeColor("dialog_fg"), element))) | 
           ftxui::color(GetThemeColor("dialog_border"));
}

ftxui::Element ThemeManager::CreateButtonStyle(ftxui::Element element) const {
    return ftxui::bgcolor(GetThemeColor("button_bg"), 
           ftxui::color(GetThemeColor("button_fg"), element));
}

ftxui::Element ThemeManager::CreateInputStyle(ftxui::Element element) const {
    return ftxui::bgcolor(GetThemeColor("input_bg"), 
           ftxui::color(GetThemeColor("input_fg"), element));
}

void ThemeManager::ReloadTheme() {
    ApplyTheme(current_theme_);
}

const FTBConfig& ThemeManager::GetThemeConfig() const {
    return theme_config_;
}

void ThemeManager::InitializePredefinedThemes() {
    // 默认主题
    FTBConfig default_theme;
    default_theme.colors_main.background = "black";
    default_theme.colors_main.foreground = "white";
    default_theme.colors_main.border = "blue";
    default_theme.colors_main.selection_bg = "blue";
    default_theme.colors_main.selection_fg = "white";
    
    default_theme.colors_files.directory = "blue";
    default_theme.colors_files.file = "white";
    default_theme.colors_files.executable = "green";
    default_theme.colors_files.link = "cyan";
    default_theme.colors_files.hidden = "yellow";
    default_theme.colors_files.system = "red";
    
    default_theme.colors_status.background = "blue";
    default_theme.colors_status.foreground = "white";
    
    default_theme.colors_search.background = "black";
    default_theme.colors_search.foreground = "white";
    default_theme.colors_search.border = "green";
    
    default_theme.colors_dialog.background = "black";
    default_theme.colors_dialog.foreground = "white";
    default_theme.colors_dialog.border = "blue";
    
    predefined_themes_["default"] = default_theme;
    
    // 暗色主题
    FTBConfig dark_theme = default_theme;
    dark_theme.colors_main.background = "black";
    dark_theme.colors_main.foreground = "white";
    dark_theme.colors_main.border = "gray";
    dark_theme.colors_main.selection_bg = "dark_gray";
    dark_theme.colors_main.selection_fg = "white";
    
    dark_theme.colors_files.directory = "cyan";
    dark_theme.colors_files.file = "white";
    dark_theme.colors_files.executable = "green";
    dark_theme.colors_files.link = "yellow";
    dark_theme.colors_files.hidden = "dark_gray";
    dark_theme.colors_files.system = "red";
    
    predefined_themes_["dark"] = dark_theme;
    
    // 亮色主题
    FTBConfig light_theme = default_theme;
    light_theme.colors_main.background = "white";
    light_theme.colors_main.foreground = "black";
    light_theme.colors_main.border = "dark_gray";
    light_theme.colors_main.selection_bg = "blue";
    light_theme.colors_main.selection_fg = "white";
    
    light_theme.colors_files.directory = "blue";
    light_theme.colors_files.file = "black";
    light_theme.colors_files.executable = "green";
    light_theme.colors_files.link = "cyan";
    light_theme.colors_files.hidden = "dark_gray";
    light_theme.colors_files.system = "red";
    
    predefined_themes_["light"] = light_theme;
    
    // 彩色主题
    FTBConfig colorful_theme = default_theme;
    colorful_theme.colors_main.background = "black";
    colorful_theme.colors_main.foreground = "white";
    colorful_theme.colors_main.border = "magenta";
    colorful_theme.colors_main.selection_bg = "magenta";
    colorful_theme.colors_main.selection_fg = "white";
    
    colorful_theme.colors_files.directory = "cyan";
    colorful_theme.colors_files.file = "white";
    colorful_theme.colors_files.executable = "green";
    colorful_theme.colors_files.link = "yellow";
    colorful_theme.colors_files.hidden = "magenta";
    colorful_theme.colors_files.system = "red";
    
    colorful_theme.colors_status.background = "magenta";
    colorful_theme.colors_status.foreground = "white";
    
    colorful_theme.colors_search.background = "black";
    colorful_theme.colors_search.foreground = "white";
    colorful_theme.colors_search.border = "magenta";
    
    predefined_themes_["colorful"] = colorful_theme;
    
    // 极简主题
    FTBConfig minimal_theme = default_theme;
    minimal_theme.colors_main.background = "black";
    minimal_theme.colors_main.foreground = "white";
    minimal_theme.colors_main.border = "white";
    minimal_theme.colors_main.selection_bg = "white";
    minimal_theme.colors_main.selection_fg = "black";
    
    minimal_theme.colors_files.directory = "white";
    minimal_theme.colors_files.file = "white";
    minimal_theme.colors_files.executable = "white";
    minimal_theme.colors_files.link = "white";
    minimal_theme.colors_files.hidden = "white";
    minimal_theme.colors_files.system = "white";
    
    minimal_theme.style.show_icons = false;
    minimal_theme.style.enable_animations = false;
    
    predefined_themes_["minimal"] = minimal_theme;
}

void ThemeManager::ApplyThemeConfig(const std::string& theme_name) {
    auto it = predefined_themes_.find(theme_name);
    if (it != predefined_themes_.end()) {
        theme_config_ = it->second;
    } else {
        // 如果主题不存在，使用默认主题
        theme_config_ = predefined_themes_["default"];
        std::cerr << "警告: 主题 '" << theme_name << "' 不存在，使用默认主题" << std::endl;
    }
}

void ThemeManager::CreateThemeColorMap(const std::string& theme_name) {
    current_theme_colors_.clear();
    
    // 主界面颜色
    current_theme_colors_["main_bg"] = ftxui::Color::Black;
    current_theme_colors_["main_fg"] = ftxui::Color::White;
    current_theme_colors_["main_border"] = ftxui::Color::Blue;
    current_theme_colors_["selection_bg"] = ftxui::Color::Blue;
    current_theme_colors_["selection_fg"] = ftxui::Color::White;
    
    // 文件类型颜色
    current_theme_colors_["directory"] = ftxui::Color::Blue;
    current_theme_colors_["file"] = ftxui::Color::White;
    current_theme_colors_["executable"] = ftxui::Color::Green;
    current_theme_colors_["link"] = ftxui::Color::Cyan;
    current_theme_colors_["hidden"] = ftxui::Color::Yellow;
    current_theme_colors_["system"] = ftxui::Color::Red;
    
    // 状态栏颜色
    current_theme_colors_["status_bg"] = ftxui::Color::Blue;
    current_theme_colors_["status_fg"] = ftxui::Color::White;
    current_theme_colors_["time"] = ftxui::Color::Yellow;
    current_theme_colors_["path"] = ftxui::Color::Cyan;
    
    // 搜索框颜色
    current_theme_colors_["search_bg"] = ftxui::Color::Black;
    current_theme_colors_["search_fg"] = ftxui::Color::White;
    current_theme_colors_["search_border"] = ftxui::Color::Green;
    current_theme_colors_["search_highlight"] = ftxui::Color::Yellow;
    
    // 对话框颜色
    current_theme_colors_["dialog_bg"] = ftxui::Color::Black;
    current_theme_colors_["dialog_fg"] = ftxui::Color::White;
    current_theme_colors_["dialog_border"] = ftxui::Color::Blue;
    current_theme_colors_["button_bg"] = ftxui::Color::Blue;
    current_theme_colors_["button_fg"] = ftxui::Color::White;
    current_theme_colors_["input_bg"] = ftxui::Color::Black;
    current_theme_colors_["input_fg"] = ftxui::Color::White;
    
    // 根据主题配置更新颜色
    if (theme_name == "dark") {
        current_theme_colors_["main_border"] = ftxui::Color::GrayDark;
        current_theme_colors_["selection_bg"] = ftxui::Color::GrayDark;
        current_theme_colors_["hidden"] = ftxui::Color::GrayDark;
    }
    else if (theme_name == "light") {
        current_theme_colors_["main_bg"] = ftxui::Color::White;
        current_theme_colors_["main_fg"] = ftxui::Color::Black;
        current_theme_colors_["main_border"] = ftxui::Color::GrayDark;
        current_theme_colors_["file"] = ftxui::Color::Black;
        current_theme_colors_["hidden"] = ftxui::Color::GrayDark;
    }
    else if (theme_name == "colorful") {
        current_theme_colors_["main_border"] = ftxui::Color::Magenta;
        current_theme_colors_["selection_bg"] = ftxui::Color::Magenta;
        current_theme_colors_["status_bg"] = ftxui::Color::Magenta;
        current_theme_colors_["search_border"] = ftxui::Color::Magenta;
        current_theme_colors_["dialog_border"] = ftxui::Color::Magenta;
        current_theme_colors_["button_bg"] = ftxui::Color::Magenta;
        current_theme_colors_["hidden"] = ftxui::Color::Magenta;
    }
    else if (theme_name == "minimal") {
        current_theme_colors_["main_border"] = ftxui::Color::White;
        current_theme_colors_["selection_bg"] = ftxui::Color::White;
        current_theme_colors_["selection_fg"] = ftxui::Color::Black;
        current_theme_colors_["directory"] = ftxui::Color::White;
        current_theme_colors_["file"] = ftxui::Color::White;
        current_theme_colors_["executable"] = ftxui::Color::White;
        current_theme_colors_["link"] = ftxui::Color::White;
        current_theme_colors_["hidden"] = ftxui::Color::White;
        current_theme_colors_["system"] = ftxui::Color::White;
    }
}

} // namespace FTB 