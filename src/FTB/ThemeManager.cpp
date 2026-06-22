#include "FTB/ThemeManager.hpp"
#include <iostream>
#include <algorithm>

namespace FTB {

std::unique_ptr<ThemeManager> ThemeManager::instance_ = nullptr;

ThemeManager::ThemeManager() : current_theme_("") {
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
    // ---- yazi 风格主题 (Catppuccin Mocha 色系) ----
    FTBConfig yazi_theme;
    yazi_theme.colors_main.background = "#1e1e2e";
    yazi_theme.colors_main.foreground = "#cdd6f4";
    yazi_theme.colors_main.border = "#45475a";
    yazi_theme.colors_main.selection_bg = "#585b70";
    yazi_theme.colors_main.selection_fg = "#cdd6f4";

    yazi_theme.colors_files.directory = "#89b4fa";
    yazi_theme.colors_files.file = "#cdd6f4";
    yazi_theme.colors_files.executable = "#a6e3a1";
    yazi_theme.colors_files.link = "#f5c2e7";
    yazi_theme.colors_files.hidden = "#6c7086";
    yazi_theme.colors_files.system = "#f38ba8";

    yazi_theme.colors_status.background = "#313244";
    yazi_theme.colors_status.foreground = "#cdd6f4";
    yazi_theme.colors_status.border = "#45475a";

    yazi_theme.colors_search.background = "#1e1e2e";
    yazi_theme.colors_search.foreground = "#cdd6f4";
    yazi_theme.colors_search.border = "#89b4fa";

    yazi_theme.colors_dialog.background = "#1e1e2e";
    yazi_theme.colors_dialog.foreground = "#cdd6f4";
    yazi_theme.colors_dialog.border = "#89b4fa";

    yazi_theme.colors_syntax.keyword = "#cba6f7";
    yazi_theme.colors_syntax.string = "#a6e3a1";
    yazi_theme.colors_syntax.comment = "#6c7086";
    yazi_theme.colors_syntax.number = "#fab387";
    yazi_theme.colors_syntax.function = "#89b4fa";
    yazi_theme.colors_syntax.type = "#f9e2af";
    yazi_theme.colors_syntax.operator_ = "#89dceb";
    yazi_theme.colors_syntax.preprocessor = "#f38ba8";
    yazi_theme.colors_syntax.identifier = "#cdd6f4";
    yazi_theme.colors_syntax.punctuation = "#6c7086";
    yazi_theme.colors_syntax.property = "#89dceb";
    yazi_theme.colors_syntax.tag = "#cba6f7";
    yazi_theme.colors_syntax.attribute = "#f9e2af";
    yazi_theme.colors_syntax.regex = "#fab387";
    yazi_theme.colors_syntax.decorator = "#f38ba8";
    yazi_theme.colors_syntax.line_number = "#585b70";

    yazi_theme.style.show_icons = true;
    yazi_theme.style.show_file_size = true;
    yazi_theme.style.show_modified_time = true;
    yazi_theme.style.show_permissions = true;
    yazi_theme.style.enable_mouse = true;
    yazi_theme.style.enable_animations = false;

    predefined_themes_["yazi"] = yazi_theme;

    // 默认主题 (保持 yazi 风格)
    FTBConfig default_theme = yazi_theme;
    predefined_themes_["default"] = default_theme;

    // 暗色主题 (yazi dark variant)
    FTBConfig dark_theme = yazi_theme;
    dark_theme.colors_main.background = "#11111b";
    dark_theme.colors_main.foreground = "#cdd6f4";
    dark_theme.colors_main.border = "#313244";
    dark_theme.colors_main.selection_bg = "#45475a";
    dark_theme.colors_main.selection_fg = "#cdd6f4";
    dark_theme.colors_files.hidden = "#585b70";
    dark_theme.colors_status.background = "#181825";
    dark_theme.colors_status.foreground = "#cdd6f4";
    predefined_themes_["dark"] = dark_theme;

    // 亮色主题 (Catppuccin Latte)
    FTBConfig light_theme;
    light_theme.colors_main.background = "#eff1f5";
    light_theme.colors_main.foreground = "#4c4f69";
    light_theme.colors_main.border = "#bcc0cc";
    light_theme.colors_main.selection_bg = "#ccd0da";
    light_theme.colors_main.selection_fg = "#4c4f69";

    light_theme.colors_files.directory = "#1e66f5";
    light_theme.colors_files.file = "#4c4f69";
    light_theme.colors_files.executable = "#40a02b";
    light_theme.colors_files.link = "#ea76cb";
    light_theme.colors_files.hidden = "#9ca0b0";
    light_theme.colors_files.system = "#d20f39";

    light_theme.colors_status.background = "#e6e9ef";
    light_theme.colors_status.foreground = "#4c4f69";
    light_theme.colors_status.border = "#bcc0cc";

    light_theme.colors_search.background = "#eff1f5";
    light_theme.colors_search.foreground = "#4c4f69";
    light_theme.colors_search.border = "#1e66f5";

    light_theme.colors_dialog.background = "#eff1f5";
    light_theme.colors_dialog.foreground = "#4c4f69";
    light_theme.colors_dialog.border = "#1e66f5";

    light_theme.colors_syntax.keyword = "#8839ef";
    light_theme.colors_syntax.string = "#40a02b";
    light_theme.colors_syntax.comment = "#9ca0b0";
    light_theme.colors_syntax.number = "#fe640b";
    light_theme.colors_syntax.function = "#1e66f5";
    light_theme.colors_syntax.type = "#df8e1d";
    light_theme.colors_syntax.operator_ = "#04a5e5";
    light_theme.colors_syntax.preprocessor = "#d20f39";
    light_theme.colors_syntax.identifier = "#4c4f69";
    light_theme.colors_syntax.punctuation = "#9ca0b0";
    light_theme.colors_syntax.property = "#04a5e5";
    light_theme.colors_syntax.tag = "#8839ef";
    light_theme.colors_syntax.attribute = "#df8e1d";
    light_theme.colors_syntax.regex = "#fe640b";
    light_theme.colors_syntax.decorator = "#d20f39";
    light_theme.colors_syntax.line_number = "#9ca0b0";

    predefined_themes_["light"] = light_theme;

    // 彩色主题 (yazi variant with more color)
    FTBConfig colorful_theme = yazi_theme;
    colorful_theme.colors_main.border = "#cba6f7";
    colorful_theme.colors_main.selection_bg = "#cba6f7";
    colorful_theme.colors_main.selection_fg = "#1e1e2e";
    colorful_theme.colors_status.background = "#cba6f7";
    colorful_theme.colors_status.foreground = "#1e1e2e";
    colorful_theme.colors_search.border = "#cba6f7";
    colorful_theme.colors_dialog.border = "#cba6f7";
    colorful_theme.colors_files.hidden = "#cba6f7";
    predefined_themes_["colorful"] = colorful_theme;

    // 极简主题
    FTBConfig minimal_theme = yazi_theme;
    minimal_theme.colors_main.border = "#585b70";
    minimal_theme.colors_main.selection_bg = "#585b70";
    minimal_theme.colors_main.selection_fg = "#cdd6f4";
    minimal_theme.colors_files.directory = "#cdd6f4";
    minimal_theme.colors_files.file = "#cdd6f4";
    minimal_theme.colors_files.executable = "#cdd6f4";
    minimal_theme.colors_files.link = "#cdd6f4";
    minimal_theme.colors_files.hidden = "#585b70";
    minimal_theme.colors_files.system = "#cdd6f4";
    minimal_theme.style.show_icons = false;
    minimal_theme.style.enable_animations = false;
    predefined_themes_["minimal"] = minimal_theme;

    // ---- Dracula ----
    FTBConfig dracula_theme;
    dracula_theme.colors_main.background = "#282a36";
    dracula_theme.colors_main.foreground = "#f8f8f2";
    dracula_theme.colors_main.border = "#44475a";
    dracula_theme.colors_main.selection_bg = "#44475a";
    dracula_theme.colors_main.selection_fg = "#f8f8f2";
    dracula_theme.colors_files.directory = "#8be9fd";
    dracula_theme.colors_files.file = "#f8f8f2";
    dracula_theme.colors_files.executable = "#50fa7b";
    dracula_theme.colors_files.link = "#ff79c6";
    dracula_theme.colors_files.hidden = "#6272a4";
    dracula_theme.colors_files.system = "#ff5555";
    dracula_theme.colors_status.background = "#44475a";
    dracula_theme.colors_status.foreground = "#f8f8f2";
    dracula_theme.colors_status.border = "#44475a";
    dracula_theme.colors_search.background = "#282a36";
    dracula_theme.colors_search.foreground = "#f8f8f2";
    dracula_theme.colors_search.border = "#bd93f9";
    dracula_theme.colors_dialog.background = "#282a36";
    dracula_theme.colors_dialog.foreground = "#f8f8f2";
    dracula_theme.colors_dialog.border = "#bd93f9";

    dracula_theme.colors_syntax.keyword = "#ff79c6";
    dracula_theme.colors_syntax.string = "#f1fa8c";
    dracula_theme.colors_syntax.comment = "#6272a4";
    dracula_theme.colors_syntax.number = "#bd93f9";
    dracula_theme.colors_syntax.function = "#50fa7b";
    dracula_theme.colors_syntax.type = "#8be9fd";
    dracula_theme.colors_syntax.operator_ = "#ff79c6";
    dracula_theme.colors_syntax.preprocessor = "#ff5555";
    dracula_theme.colors_syntax.identifier = "#f8f8f2";
    dracula_theme.colors_syntax.punctuation = "#6272a4";
    dracula_theme.colors_syntax.property = "#bd93f9";
    dracula_theme.colors_syntax.tag = "#ff79c6";
    dracula_theme.colors_syntax.attribute = "#50fa7b";
    dracula_theme.colors_syntax.regex = "#f1fa8c";
    dracula_theme.colors_syntax.decorator = "#50fa7b";
    dracula_theme.colors_syntax.line_number = "#6272a4";

    predefined_themes_["dracula"] = dracula_theme;

    // ---- Nord ----
    FTBConfig nord_theme;
    nord_theme.colors_main.background = "#2e3440";
    nord_theme.colors_main.foreground = "#d8dee9";
    nord_theme.colors_main.border = "#3b4252";
    nord_theme.colors_main.selection_bg = "#434c5e";
    nord_theme.colors_main.selection_fg = "#d8dee9";
    nord_theme.colors_files.directory = "#88c0d0";
    nord_theme.colors_files.file = "#d8dee9";
    nord_theme.colors_files.executable = "#a3be8c";
    nord_theme.colors_files.link = "#b48ead";
    nord_theme.colors_files.hidden = "#4c566a";
    nord_theme.colors_files.system = "#bf616a";
    nord_theme.colors_status.background = "#3b4252";
    nord_theme.colors_status.foreground = "#d8dee9";
    nord_theme.colors_status.border = "#3b4252";
    nord_theme.colors_search.background = "#2e3440";
    nord_theme.colors_search.foreground = "#d8dee9";
    nord_theme.colors_search.border = "#88c0d0";
    nord_theme.colors_dialog.background = "#2e3440";
    nord_theme.colors_dialog.foreground = "#d8dee9";
    nord_theme.colors_dialog.border = "#88c0d0";

    nord_theme.colors_syntax.keyword = "#81a1c1";
    nord_theme.colors_syntax.string = "#a3be8c";
    nord_theme.colors_syntax.comment = "#4c566a";
    nord_theme.colors_syntax.number = "#b48ead";
    nord_theme.colors_syntax.function = "#88c0d0";
    nord_theme.colors_syntax.type = "#8fbcbb";
    nord_theme.colors_syntax.operator_ = "#81a1c1";
    nord_theme.colors_syntax.preprocessor = "#bf616a";
    nord_theme.colors_syntax.identifier = "#d8dee9";
    nord_theme.colors_syntax.punctuation = "#4c566a";
    nord_theme.colors_syntax.property = "#88c0d0";
    nord_theme.colors_syntax.tag = "#81a1c1";
    nord_theme.colors_syntax.attribute = "#8fbcbb";
    nord_theme.colors_syntax.regex = "#ebcb8b";
    nord_theme.colors_syntax.decorator = "#b48ead";
    nord_theme.colors_syntax.line_number = "#4c566a";

    predefined_themes_["nord"] = nord_theme;

    // ---- Tokyo Night ----
    FTBConfig tokyo_theme;
    tokyo_theme.colors_main.background = "#1a1b26";
    tokyo_theme.colors_main.foreground = "#a9b1d6";
    tokyo_theme.colors_main.border = "#292e42";
    tokyo_theme.colors_main.selection_bg = "#33467c";
    tokyo_theme.colors_main.selection_fg = "#a9b1d6";
    tokyo_theme.colors_files.directory = "#7aa2f7";
    tokyo_theme.colors_files.file = "#a9b1d6";
    tokyo_theme.colors_files.executable = "#9ece6a";
    tokyo_theme.colors_files.link = "#bb9af7";
    tokyo_theme.colors_files.hidden = "#565f89";
    tokyo_theme.colors_files.system = "#f7768e";
    tokyo_theme.colors_status.background = "#292e42";
    tokyo_theme.colors_status.foreground = "#a9b1d6";
    tokyo_theme.colors_status.border = "#292e42";
    tokyo_theme.colors_search.background = "#1a1b26";
    tokyo_theme.colors_search.foreground = "#a9b1d6";
    tokyo_theme.colors_search.border = "#7aa2f7";
    tokyo_theme.colors_dialog.background = "#1a1b26";
    tokyo_theme.colors_dialog.foreground = "#a9b1d6";
    tokyo_theme.colors_dialog.border = "#7aa2f7";

    tokyo_theme.colors_syntax.keyword = "#bb9af7";
    tokyo_theme.colors_syntax.string = "#9ece6a";
    tokyo_theme.colors_syntax.comment = "#565f89";
    tokyo_theme.colors_syntax.number = "#ff9e64";
    tokyo_theme.colors_syntax.function = "#7aa2f7";
    tokyo_theme.colors_syntax.type = "#e0af68";
    tokyo_theme.colors_syntax.operator_ = "#89ddff";
    tokyo_theme.colors_syntax.preprocessor = "#f7768e";
    tokyo_theme.colors_syntax.identifier = "#a9b1d6";
    tokyo_theme.colors_syntax.punctuation = "#565f89";
    tokyo_theme.colors_syntax.property = "#73daca";
    tokyo_theme.colors_syntax.tag = "#bb9af7";
    tokyo_theme.colors_syntax.attribute = "#e0af68";
    tokyo_theme.colors_syntax.regex = "#ff9e64";
    tokyo_theme.colors_syntax.decorator = "#f7768e";
    tokyo_theme.colors_syntax.line_number = "#3b4261";

    predefined_themes_["tokyo-night"] = tokyo_theme;

    // ---- Gruvbox Dark ----
    FTBConfig gruvbox_theme;
    gruvbox_theme.colors_main.background = "#282828";
    gruvbox_theme.colors_main.foreground = "#ebdbb2";
    gruvbox_theme.colors_main.border = "#3c3836";
    gruvbox_theme.colors_main.selection_bg = "#504945";
    gruvbox_theme.colors_main.selection_fg = "#ebdbb2";
    gruvbox_theme.colors_files.directory = "#83a598";
    gruvbox_theme.colors_files.file = "#ebdbb2";
    gruvbox_theme.colors_files.executable = "#b8bb26";
    gruvbox_theme.colors_files.link = "#d3869b";
    gruvbox_theme.colors_files.hidden = "#665c54";
    gruvbox_theme.colors_files.system = "#fb4934";
    gruvbox_theme.colors_status.background = "#3c3836";
    gruvbox_theme.colors_status.foreground = "#ebdbb2";
    gruvbox_theme.colors_status.border = "#3c3836";
    gruvbox_theme.colors_search.background = "#282828";
    gruvbox_theme.colors_search.foreground = "#ebdbb2";
    gruvbox_theme.colors_search.border = "#fe8019";
    gruvbox_theme.colors_dialog.background = "#282828";
    gruvbox_theme.colors_dialog.foreground = "#ebdbb2";
    gruvbox_theme.colors_dialog.border = "#fe8019";

    gruvbox_theme.colors_syntax.keyword = "#fe8019";
    gruvbox_theme.colors_syntax.string = "#b8bb26";
    gruvbox_theme.colors_syntax.comment = "#665c54";
    gruvbox_theme.colors_syntax.number = "#d3869b";
    gruvbox_theme.colors_syntax.function = "#fabd2f";
    gruvbox_theme.colors_syntax.type = "#83a598";
    gruvbox_theme.colors_syntax.operator_ = "#fe8019";
    gruvbox_theme.colors_syntax.preprocessor = "#fb4934";
    gruvbox_theme.colors_syntax.identifier = "#ebdbb2";
    gruvbox_theme.colors_syntax.punctuation = "#665c54";
    gruvbox_theme.colors_syntax.property = "#83a598";
    gruvbox_theme.colors_syntax.tag = "#fe8019";
    gruvbox_theme.colors_syntax.attribute = "#fabd2f";
    gruvbox_theme.colors_syntax.regex = "#d3869b";
    gruvbox_theme.colors_syntax.decorator = "#fb4934";
    gruvbox_theme.colors_syntax.line_number = "#504945";

    predefined_themes_["gruvbox"] = gruvbox_theme;

    // ---- Solarized Dark ----
    FTBConfig solarized_theme;
    solarized_theme.colors_main.background = "#002b36";
    solarized_theme.colors_main.foreground = "#839496";
    solarized_theme.colors_main.border = "#073642";
    solarized_theme.colors_main.selection_bg = "#073642";
    solarized_theme.colors_main.selection_fg = "#93a1a1";
    solarized_theme.colors_files.directory = "#268bd2";
    solarized_theme.colors_files.file = "#839496";
    solarized_theme.colors_files.executable = "#859900";
    solarized_theme.colors_files.link = "#6c71c4";
    solarized_theme.colors_files.hidden = "#586e75";
    solarized_theme.colors_files.system = "#dc322f";
    solarized_theme.colors_status.background = "#073642";
    solarized_theme.colors_status.foreground = "#839496";
    solarized_theme.colors_status.border = "#073642";
    solarized_theme.colors_search.background = "#002b36";
    solarized_theme.colors_search.foreground = "#839496";
    solarized_theme.colors_search.border = "#268bd2";
    solarized_theme.colors_dialog.background = "#002b36";
    solarized_theme.colors_dialog.foreground = "#839496";
    solarized_theme.colors_dialog.border = "#268bd2";

    solarized_theme.colors_syntax.keyword = "#859900";
    solarized_theme.colors_syntax.string = "#2aa198";
    solarized_theme.colors_syntax.comment = "#586e75";
    solarized_theme.colors_syntax.number = "#d33682";
    solarized_theme.colors_syntax.function = "#268bd2";
    solarized_theme.colors_syntax.type = "#b58900";
    solarized_theme.colors_syntax.operator_ = "#859900";
    solarized_theme.colors_syntax.preprocessor = "#dc322f";
    solarized_theme.colors_syntax.identifier = "#839496";
    solarized_theme.colors_syntax.punctuation = "#586e75";
    solarized_theme.colors_syntax.property = "#268bd2";
    solarized_theme.colors_syntax.tag = "#859900";
    solarized_theme.colors_syntax.attribute = "#b58900";
    solarized_theme.colors_syntax.regex = "#d33682";
    solarized_theme.colors_syntax.decorator = "#dc322f";
    solarized_theme.colors_syntax.line_number = "#586e75";

    predefined_themes_["solarized"] = solarized_theme;

    // ---- One Dark ----
    FTBConfig onedark_theme;
    onedark_theme.colors_main.background = "#282c34";
    onedark_theme.colors_main.foreground = "#abb2bf";
    onedark_theme.colors_main.border = "#3e4451";
    onedark_theme.colors_main.selection_bg = "#3e4451";
    onedark_theme.colors_main.selection_fg = "#abb2bf";
    onedark_theme.colors_files.directory = "#61afef";
    onedark_theme.colors_files.file = "#abb2bf";
    onedark_theme.colors_files.executable = "#98c379";
    onedark_theme.colors_files.link = "#c678dd";
    onedark_theme.colors_files.hidden = "#5c6370";
    onedark_theme.colors_files.system = "#e06c75";
    onedark_theme.colors_status.background = "#2c313c";
    onedark_theme.colors_status.foreground = "#abb2bf";
    onedark_theme.colors_status.border = "#3e4451";
    onedark_theme.colors_search.background = "#282c34";
    onedark_theme.colors_search.foreground = "#abb2bf";
    onedark_theme.colors_search.border = "#61afef";
    onedark_theme.colors_dialog.background = "#282c34";
    onedark_theme.colors_dialog.foreground = "#abb2bf";
    onedark_theme.colors_dialog.border = "#61afef";

    onedark_theme.colors_syntax.keyword = "#c678dd";
    onedark_theme.colors_syntax.string = "#98c379";
    onedark_theme.colors_syntax.comment = "#5c6370";
    onedark_theme.colors_syntax.number = "#d19a66";
    onedark_theme.colors_syntax.function = "#61afef";
    onedark_theme.colors_syntax.type = "#e5c07b";
    onedark_theme.colors_syntax.operator_ = "#56b6c2";
    onedark_theme.colors_syntax.preprocessor = "#e06c75";
    onedark_theme.colors_syntax.identifier = "#abb2bf";
    onedark_theme.colors_syntax.punctuation = "#5c6370";
    onedark_theme.colors_syntax.property = "#e06c75";
    onedark_theme.colors_syntax.tag = "#e06c75";
    onedark_theme.colors_syntax.attribute = "#d19a66";
    onedark_theme.colors_syntax.regex = "#d19a66";
    onedark_theme.colors_syntax.decorator = "#e06c75";
    onedark_theme.colors_syntax.line_number = "#4b5263";

    predefined_themes_["one-dark"] = onedark_theme;

    // ---- Rose Pine ----
    FTBConfig rosepine_theme;
    rosepine_theme.colors_main.background = "#191724";
    rosepine_theme.colors_main.foreground = "#e0def4";
    rosepine_theme.colors_main.border = "#26233a";
    rosepine_theme.colors_main.selection_bg = "#26233a";
    rosepine_theme.colors_main.selection_fg = "#e0def4";
    rosepine_theme.colors_files.directory = "#c4a7e7";
    rosepine_theme.colors_files.file = "#e0def4";
    rosepine_theme.colors_files.executable = "#9ccfd8";
    rosepine_theme.colors_files.link = "#ebbcba";
    rosepine_theme.colors_files.hidden = "#6e6a86";
    rosepine_theme.colors_files.system = "#eb6f92";
    rosepine_theme.colors_status.background = "#1f1d2e";
    rosepine_theme.colors_status.foreground = "#e0def4";
    rosepine_theme.colors_status.border = "#26233a";
    rosepine_theme.colors_search.background = "#191724";
    rosepine_theme.colors_search.foreground = "#e0def4";
    rosepine_theme.colors_search.border = "#c4a7e7";
    rosepine_theme.colors_dialog.background = "#191724";
    rosepine_theme.colors_dialog.foreground = "#e0def4";
    rosepine_theme.colors_dialog.border = "#c4a7e7";

    rosepine_theme.colors_syntax.keyword = "#31748f";
    rosepine_theme.colors_syntax.string = "#a6e3a1";
    rosepine_theme.colors_syntax.comment = "#6e6a86";
    rosepine_theme.colors_syntax.number = "#ebbcba";
    rosepine_theme.colors_syntax.function = "#c4a7e7";
    rosepine_theme.colors_syntax.type = "#9ccfd8";
    rosepine_theme.colors_syntax.operator_ = "#31748f";
    rosepine_theme.colors_syntax.preprocessor = "#eb6f92";
    rosepine_theme.colors_syntax.identifier = "#e0def4";
    rosepine_theme.colors_syntax.punctuation = "#6e6a86";
    rosepine_theme.colors_syntax.property = "#9ccfd8";
    rosepine_theme.colors_syntax.tag = "#31748f";
    rosepine_theme.colors_syntax.attribute = "#ebbcba";
    rosepine_theme.colors_syntax.regex = "#ebbcba";
    rosepine_theme.colors_syntax.decorator = "#eb6f92";
    rosepine_theme.colors_syntax.line_number = "#26233a";

    predefined_themes_["rose-pine"] = rosepine_theme;

    // ---- Kanagawa ----
    FTBConfig kanagawa_theme;
    kanagawa_theme.colors_main.background = "#1f1f28";
    kanagawa_theme.colors_main.foreground = "#dcd7ba";
    kanagawa_theme.colors_main.border = "#2a2a37";
    kanagawa_theme.colors_main.selection_bg = "#2a2a37";
    kanagawa_theme.colors_main.selection_fg = "#dcd7ba";
    kanagawa_theme.colors_files.directory = "#7e9cd8";
    kanagawa_theme.colors_files.file = "#dcd7ba";
    kanagawa_theme.colors_files.executable = "#769cba";
    kanagawa_theme.colors_files.link = "#957fb8";
    kanagawa_theme.colors_files.hidden = "#54546d";
    kanagawa_theme.colors_files.system = "#c34043";
    kanagawa_theme.colors_status.background = "#16161d";
    kanagawa_theme.colors_status.foreground = "#dcd7ba";
    kanagawa_theme.colors_status.border = "#2a2a37";
    kanagawa_theme.colors_search.background = "#1f1f28";
    kanagawa_theme.colors_search.foreground = "#dcd7ba";
    kanagawa_theme.colors_search.border = "#7e9cd8";
    kanagawa_theme.colors_dialog.background = "#1f1f28";
    kanagawa_theme.colors_dialog.foreground = "#dcd7ba";
    kanagawa_theme.colors_dialog.border = "#7e9cd8";
    kanagawa_theme.colors_syntax.keyword = "#957fb8";
    kanagawa_theme.colors_syntax.string = "#769cba";
    kanagawa_theme.colors_syntax.comment = "#54546d";
    kanagawa_theme.colors_syntax.number = "#ff9e64";
    kanagawa_theme.colors_syntax.function = "#7e9cd8";
    kanagawa_theme.colors_syntax.type = "#e6c384";
    kanagawa_theme.colors_syntax.operator_ = "#c0a36e";
    kanagawa_theme.colors_syntax.preprocessor = "#c34043";
    kanagawa_theme.colors_syntax.identifier = "#dcd7ba";
    kanagawa_theme.colors_syntax.punctuation = "#54546d";
    kanagawa_theme.colors_syntax.property = "#769cba";
    kanagawa_theme.colors_syntax.tag = "#957fb8";
    kanagawa_theme.colors_syntax.attribute = "#e6c384";
    kanagawa_theme.colors_syntax.regex = "#ff9e64";
    kanagawa_theme.colors_syntax.decorator = "#c34043";
    kanagawa_theme.colors_syntax.line_number = "#2a2a37";
    predefined_themes_["kanagawa"] = kanagawa_theme;

    // ---- Everforest ----
    FTBConfig everforest_theme;
    everforest_theme.colors_main.background = "#2d353b";
    everforest_theme.colors_main.foreground = "#d3c6aa";
    everforest_theme.colors_main.border = "#414b50";
    everforest_theme.colors_main.selection_bg = "#414b50";
    everforest_theme.colors_main.selection_fg = "#d3c6aa";
    everforest_theme.colors_files.directory = "#7fbbb3";
    everforest_theme.colors_files.file = "#d3c6aa";
    everforest_theme.colors_files.executable = "#a7c080";
    everforest_theme.colors_files.link = "#d699b6";
    everforest_theme.colors_files.hidden = "#5c6a72";
    everforest_theme.colors_files.system = "#e67e80";
    everforest_theme.colors_status.background = "#232a2e";
    everforest_theme.colors_status.foreground = "#d3c6aa";
    everforest_theme.colors_status.border = "#414b50";
    everforest_theme.colors_search.background = "#2d353b";
    everforest_theme.colors_search.foreground = "#d3c6aa";
    everforest_theme.colors_search.border = "#7fbbb3";
    everforest_theme.colors_dialog.background = "#2d353b";
    everforest_theme.colors_dialog.foreground = "#d3c6aa";
    everforest_theme.colors_dialog.border = "#7fbbb3";
    everforest_theme.colors_syntax.keyword = "#e67e80";
    everforest_theme.colors_syntax.string = "#a7c080";
    everforest_theme.colors_syntax.comment = "#5c6a72";
    everforest_theme.colors_syntax.number = "#e69875";
    everforest_theme.colors_syntax.function = "#7fbbb3";
    everforest_theme.colors_syntax.type = "#dbbc7f";
    everforest_theme.colors_syntax.operator_ = "#e69875";
    everforest_theme.colors_syntax.preprocessor = "#e67e80";
    everforest_theme.colors_syntax.identifier = "#d3c6aa";
    everforest_theme.colors_syntax.punctuation = "#5c6a72";
    everforest_theme.colors_syntax.property = "#7fbbb3";
    everforest_theme.colors_syntax.tag = "#e67e80";
    everforest_theme.colors_syntax.attribute = "#dbbc7f";
    everforest_theme.colors_syntax.regex = "#e69875";
    everforest_theme.colors_syntax.decorator = "#d699b6";
    everforest_theme.colors_syntax.line_number = "#414b50";
    predefined_themes_["everforest"] = everforest_theme;

    // ---- Monokai ----
    FTBConfig monokai_theme;
    monokai_theme.colors_main.background = "#272822";
    monokai_theme.colors_main.foreground = "#f8f8f2";
    monokai_theme.colors_main.border = "#3e3d32";
    monokai_theme.colors_main.selection_bg = "#49483e";
    monokai_theme.colors_main.selection_fg = "#f8f8f2";
    monokai_theme.colors_files.directory = "#66d9ef";
    monokai_theme.colors_files.file = "#f8f8f2";
    monokai_theme.colors_files.executable = "#a6e22e";
    monokai_theme.colors_files.link = "#ae81ff";
    monokai_theme.colors_files.hidden = "#75715e";
    monokai_theme.colors_files.system = "#f92672";
    monokai_theme.colors_status.background = "#3e3d32";
    monokai_theme.colors_status.foreground = "#f8f8f2";
    monokai_theme.colors_status.border = "#3e3d32";
    monokai_theme.colors_search.background = "#272822";
    monokai_theme.colors_search.foreground = "#f8f8f2";
    monokai_theme.colors_search.border = "#66d9ef";
    monokai_theme.colors_dialog.background = "#272822";
    monokai_theme.colors_dialog.foreground = "#f8f8f2";
    monokai_theme.colors_dialog.border = "#66d9ef";
    monokai_theme.colors_syntax.keyword = "#f92672";
    monokai_theme.colors_syntax.string = "#e6db74";
    monokai_theme.colors_syntax.comment = "#75715e";
    monokai_theme.colors_syntax.number = "#ae81ff";
    monokai_theme.colors_syntax.function = "#a6e22e";
    monokai_theme.colors_syntax.type = "#66d9ef";
    monokai_theme.colors_syntax.operator_ = "#f92672";
    monokai_theme.colors_syntax.preprocessor = "#f92672";
    monokai_theme.colors_syntax.identifier = "#f8f8f2";
    monokai_theme.colors_syntax.punctuation = "#75715e";
    monokai_theme.colors_syntax.property = "#fd971f";
    monokai_theme.colors_syntax.tag = "#f92672";
    monokai_theme.colors_syntax.attribute = "#a6e22e";
    monokai_theme.colors_syntax.regex = "#e6db74";
    monokai_theme.colors_syntax.decorator = "#fd971f";
    monokai_theme.colors_syntax.line_number = "#49483e";
    predefined_themes_["monokai"] = monokai_theme;

    // ---- Ayu Dark ----
    FTBConfig ayu_theme;
    ayu_theme.colors_main.background = "#0a0e14";
    ayu_theme.colors_main.foreground = "#b3b1ad";
    ayu_theme.colors_main.border = "#01060e";
    ayu_theme.colors_main.selection_bg = "#1a2633";
    ayu_theme.colors_main.selection_fg = "#b3b1ad";
    ayu_theme.colors_files.directory = "#39bae6";
    ayu_theme.colors_files.file = "#b3b1ad";
    ayu_theme.colors_files.executable = "#c2d94c";
    ayu_theme.colors_files.link = "#f07178";
    ayu_theme.colors_files.hidden = "#475a6f";
    ayu_theme.colors_files.system = "#f07178";
    ayu_theme.colors_status.background = "#01060e";
    ayu_theme.colors_status.foreground = "#b3b1ad";
    ayu_theme.colors_status.border = "#01060e";
    ayu_theme.colors_search.background = "#0a0e14";
    ayu_theme.colors_search.foreground = "#b3b1ad";
    ayu_theme.colors_search.border = "#39bae6";
    ayu_theme.colors_dialog.background = "#0a0e14";
    ayu_theme.colors_dialog.foreground = "#b3b1ad";
    ayu_theme.colors_dialog.border = "#39bae6";
    ayu_theme.colors_syntax.keyword = "#ff8f40";
    ayu_theme.colors_syntax.string = "#c2d94c";
    ayu_theme.colors_syntax.comment = "#626a73";
    ayu_theme.colors_syntax.number = "#e6b450";
    ayu_theme.colors_syntax.function = "#ffb454";
    ayu_theme.colors_syntax.type = "#59c2ff";
    ayu_theme.colors_syntax.operator_ = "#f29668";
    ayu_theme.colors_syntax.preprocessor = "#f07178";
    ayu_theme.colors_syntax.identifier = "#b3b1ad";
    ayu_theme.colors_syntax.punctuation = "#626a73";
    ayu_theme.colors_syntax.property = "#39bae6";
    ayu_theme.colors_syntax.tag = "#39bae6";
    ayu_theme.colors_syntax.attribute = "#ffb454";
    ayu_theme.colors_syntax.regex = "#95e6cb";
    ayu_theme.colors_syntax.decorator = "#e6b450";
    ayu_theme.colors_syntax.line_number = "#2a2e34";
    predefined_themes_["ayu"] = ayu_theme;

    // ---- Poimandres ----
    FTBConfig poimandres_theme;
    poimandres_theme.colors_main.background = "#1a1b26";
    poimandres_theme.colors_main.foreground = "#a9b1d6";
    poimandres_theme.colors_main.border = "#28345a";
    poimandres_theme.colors_main.selection_bg = "#28345a";
    poimandres_theme.colors_main.selection_fg = "#a9b1d6";
    poimandres_theme.colors_files.directory = "#7aa2f7";
    poimandres_theme.colors_files.file = "#a9b1d6";
    poimandres_theme.colors_files.executable = "#9ece6a";
    poimandres_theme.colors_files.link = "#bb9af7";
    poimandres_theme.colors_files.hidden = "#565f89";
    poimandres_theme.colors_files.system = "#f7768e";
    poimandres_theme.colors_status.background = "#16161e";
    poimandres_theme.colors_status.foreground = "#a9b1d6";
    poimandres_theme.colors_status.border = "#28345a";
    poimandres_theme.colors_search.background = "#1a1b26";
    poimandres_theme.colors_search.foreground = "#a9b1d6";
    poimandres_theme.colors_search.border = "#7aa2f7";
    poimandres_theme.colors_dialog.background = "#1a1b26";
    poimandres_theme.colors_dialog.foreground = "#a9b1d6";
    poimandres_theme.colors_dialog.border = "#7aa2f7";
    poimandres_theme.colors_syntax.keyword = "#bb9af7";
    poimandres_theme.colors_syntax.string = "#9ece6a";
    poimandres_theme.colors_syntax.comment = "#565f89";
    poimandres_theme.colors_syntax.number = "#ff9e64";
    poimandres_theme.colors_syntax.function = "#7aa2f7";
    poimandres_theme.colors_syntax.type = "#2ac3de";
    poimandres_theme.colors_syntax.operator_ = "#89ddff";
    poimandres_theme.colors_syntax.preprocessor = "#f7768e";
    poimandres_theme.colors_syntax.identifier = "#a9b1d6";
    poimandres_theme.colors_syntax.punctuation = "#565f89";
    poimandres_theme.colors_syntax.property = "#73daca";
    poimandres_theme.colors_syntax.tag = "#bb9af7";
    poimandres_theme.colors_syntax.attribute = "#e0af68";
    poimandres_theme.colors_syntax.regex = "#ff9e64";
    poimandres_theme.colors_syntax.decorator = "#f7768e";
    poimandres_theme.colors_syntax.line_number = "#3b4261";
    predefined_themes_["poimandres"] = poimandres_theme;

    // ---- Material Dark ----
    FTBConfig material_theme;
    material_theme.colors_main.background = "#1e1e2e";
    material_theme.colors_main.foreground = "#eeffff";
    material_theme.colors_main.border = "#3c3c4c";
    material_theme.colors_main.selection_bg = "#3c3c4c";
    material_theme.colors_main.selection_fg = "#eeffff";
    material_theme.colors_files.directory = "#82aaff";
    material_theme.colors_files.file = "#eeffff";
    material_theme.colors_files.executable = "#c3e88d";
    material_theme.colors_files.link = "#c792ea";
    material_theme.colors_files.hidden = "#546e7a";
    material_theme.colors_files.system = "#ff5370";
    material_theme.colors_status.background = "#1a1a2a";
    material_theme.colors_status.foreground = "#eeffff";
    material_theme.colors_status.border = "#3c3c4c";
    material_theme.colors_search.background = "#1e1e2e";
    material_theme.colors_search.foreground = "#eeffff";
    material_theme.colors_search.border = "#82aaff";
    material_theme.colors_dialog.background = "#1e1e2e";
    material_theme.colors_dialog.foreground = "#eeffff";
    material_theme.colors_dialog.border = "#82aaff";
    material_theme.colors_syntax.keyword = "#c792ea";
    material_theme.colors_syntax.string = "#c3e88d";
    material_theme.colors_syntax.comment = "#546e7a";
    material_theme.colors_syntax.number = "#f78c6c";
    material_theme.colors_syntax.function = "#82aaff";
    material_theme.colors_syntax.type = "#ffcb6b";
    material_theme.colors_syntax.operator_ = "#89ddff";
    material_theme.colors_syntax.preprocessor = "#ff5370";
    material_theme.colors_syntax.identifier = "#eeffff";
    material_theme.colors_syntax.punctuation = "#546e7a";
    material_theme.colors_syntax.property = "#f07178";
    material_theme.colors_syntax.tag = "#f07178";
    material_theme.colors_syntax.attribute = "#ffcb6b";
    material_theme.colors_syntax.regex = "#f78c6c";
    material_theme.colors_syntax.decorator = "#ff5370";
    material_theme.colors_syntax.line_number = "#3c3c4c";
    predefined_themes_["material"] = material_theme;

    // ---- Horizon ----
    FTBConfig horizon_theme;
    horizon_theme.colors_main.background = "#1c1e26";
    horizon_theme.colors_main.foreground = "#e0e0e0";
    horizon_theme.colors_main.border = "#232530";
    horizon_theme.colors_main.selection_bg = "#232530";
    horizon_theme.colors_main.selection_fg = "#e0e0e0";
    horizon_theme.colors_files.directory = "#e0e0e0";
    horizon_theme.colors_files.file = "#e0e0e0";
    horizon_theme.colors_files.executable = "#29d398";
    horizon_theme.colors_files.link = "#ee64ac";
    horizon_theme.colors_files.hidden = "#6c6f93";
    horizon_theme.colors_files.system = "#e95678";
    horizon_theme.colors_status.background = "#16161c";
    horizon_theme.colors_status.foreground = "#e0e0e0";
    horizon_theme.colors_status.border = "#232530";
    horizon_theme.colors_search.background = "#1c1e26";
    horizon_theme.colors_search.foreground = "#e0e0e0";
    horizon_theme.colors_search.border = "#e0e0e0";
    horizon_theme.colors_dialog.background = "#1c1e26";
    horizon_theme.colors_dialog.foreground = "#e0e0e0";
    horizon_theme.colors_dialog.border = "#e0e0e0";
    horizon_theme.colors_syntax.keyword = "#e95678";
    horizon_theme.colors_syntax.string = "#29d398";
    horizon_theme.colors_syntax.comment = "#6c6f93";
    horizon_theme.colors_syntax.number = "#e95678";
    horizon_theme.colors_syntax.function = "#29d398";
    horizon_theme.colors_syntax.type = "#f5937e";
    horizon_theme.colors_syntax.operator_ = "#e95678";
    horizon_theme.colors_syntax.preprocessor = "#e95678";
    horizon_theme.colors_syntax.identifier = "#e0e0e0";
    horizon_theme.colors_syntax.punctuation = "#6c6f93";
    horizon_theme.colors_syntax.property = "#29d398";
    horizon_theme.colors_syntax.tag = "#e95678";
    horizon_theme.colors_syntax.attribute = "#f5937e";
    horizon_theme.colors_syntax.regex = "#f5937e";
    horizon_theme.colors_syntax.decorator = "#ee64ac";
    horizon_theme.colors_syntax.line_number = "#232530";
    predefined_themes_["horizon"] = horizon_theme;

    // ---- Melange ----
    FTBConfig melange_theme;
    melange_theme.colors_main.background = "#292524";
    melange_theme.colors_main.foreground = "#ece1d7";
    melange_theme.colors_main.border = "#403836";
    melange_theme.colors_main.selection_bg = "#403836";
    melange_theme.colors_main.selection_fg = "#ece1d7";
    melange_theme.colors_files.directory = "#a8c5e6";
    melange_theme.colors_files.file = "#ece1d7";
    melange_theme.colors_files.executable = "#a6c7a5";
    melange_theme.colors_files.link = "#d1a4bc";
    melange_theme.colors_files.hidden = "#867462";
    melange_theme.colors_files.system = "#e8845a";
    melange_theme.colors_status.background = "#1e1c1b";
    melange_theme.colors_status.foreground = "#ece1d7";
    melange_theme.colors_status.border = "#403836";
    melange_theme.colors_search.background = "#292524";
    melange_theme.colors_search.foreground = "#ece1d7";
    melange_theme.colors_search.border = "#a8c5e6";
    melange_theme.colors_dialog.background = "#292524";
    melange_theme.colors_dialog.foreground = "#ece1d7";
    melange_theme.colors_dialog.border = "#a8c5e6";
    melange_theme.colors_syntax.keyword = "#e8845a";
    melange_theme.colors_syntax.string = "#a6c7a5";
    melange_theme.colors_syntax.comment = "#867462";
    melange_theme.colors_syntax.number = "#d1a4bc";
    melange_theme.colors_syntax.function = "#a8c5e6";
    melange_theme.colors_syntax.type = "#e4b78e";
    melange_theme.colors_syntax.operator_ = "#e8845a";
    melange_theme.colors_syntax.preprocessor = "#e8845a";
    melange_theme.colors_syntax.identifier = "#ece1d7";
    melange_theme.colors_syntax.punctuation = "#867462";
    melange_theme.colors_syntax.property = "#a8c5e6";
    melange_theme.colors_syntax.tag = "#e8845a";
    melange_theme.colors_syntax.attribute = "#e4b78e";
    melange_theme.colors_syntax.regex = "#d1a4bc";
    melange_theme.colors_syntax.decorator = "#d1a4bc";
    melange_theme.colors_syntax.line_number = "#403836";
    predefined_themes_["melange"] = melange_theme;

    // ---- Solarized Light ----
    FTBConfig solarized_light_theme;
    solarized_light_theme.colors_main.background = "#fdf6e3";
    solarized_light_theme.colors_main.foreground = "#657b83";
    solarized_light_theme.colors_main.border = "#eee8d5";
    solarized_light_theme.colors_main.selection_bg = "#eee8d5";
    solarized_light_theme.colors_main.selection_fg = "#657b83";
    solarized_light_theme.colors_files.directory = "#268bd2";
    solarized_light_theme.colors_files.file = "#657b83";
    solarized_light_theme.colors_files.executable = "#859900";
    solarized_light_theme.colors_files.link = "#6c71c4";
    solarized_light_theme.colors_files.hidden = "#93a1a1";
    solarized_light_theme.colors_files.system = "#dc322f";
    solarized_light_theme.colors_status.background = "#eee8d5";
    solarized_light_theme.colors_status.foreground = "#657b83";
    solarized_light_theme.colors_status.border = "#eee8d5";
    solarized_light_theme.colors_search.background = "#fdf6e3";
    solarized_light_theme.colors_search.foreground = "#657b83";
    solarized_light_theme.colors_search.border = "#268bd2";
    solarized_light_theme.colors_dialog.background = "#fdf6e3";
    solarized_light_theme.colors_dialog.foreground = "#657b83";
    solarized_light_theme.colors_dialog.border = "#268bd2";
    solarized_light_theme.colors_syntax.keyword = "#859900";
    solarized_light_theme.colors_syntax.string = "#2aa198";
    solarized_light_theme.colors_syntax.comment = "#93a1a1";
    solarized_light_theme.colors_syntax.number = "#d33682";
    solarized_light_theme.colors_syntax.function = "#268bd2";
    solarized_light_theme.colors_syntax.type = "#b58900";
    solarized_light_theme.colors_syntax.operator_ = "#859900";
    solarized_light_theme.colors_syntax.preprocessor = "#dc322f";
    solarized_light_theme.colors_syntax.identifier = "#657b83";
    solarized_light_theme.colors_syntax.punctuation = "#93a1a1";
    solarized_light_theme.colors_syntax.property = "#268bd2";
    solarized_light_theme.colors_syntax.tag = "#859900";
    solarized_light_theme.colors_syntax.attribute = "#b58900";
    solarized_light_theme.colors_syntax.regex = "#d33682";
    solarized_light_theme.colors_syntax.decorator = "#dc322f";
    solarized_light_theme.colors_syntax.line_number = "#93a1a1";
    predefined_themes_["solarized-light"] = solarized_light_theme;

    // ---- Catppuccin Macchiato ----
    FTBConfig macchiato_theme;
    macchiato_theme.colors_main.background = "#24273a";
    macchiato_theme.colors_main.foreground = "#cad3f5";
    macchiato_theme.colors_main.border = "#363a4f";
    macchiato_theme.colors_main.selection_bg = "#494d64";
    macchiato_theme.colors_main.selection_fg = "#cad3f5";
    macchiato_theme.colors_files.directory = "#8aadf4";
    macchiato_theme.colors_files.file = "#cad3f5";
    macchiato_theme.colors_files.executable = "#a6da95";
    macchiato_theme.colors_files.link = "#f5bde6";
    macchiato_theme.colors_files.hidden = "#5b6078";
    macchiato_theme.colors_files.system = "#ed8796";
    macchiato_theme.colors_status.background = "#1e2030";
    macchiato_theme.colors_status.foreground = "#cad3f5";
    macchiato_theme.colors_status.border = "#363a4f";
    macchiato_theme.colors_search.background = "#24273a";
    macchiato_theme.colors_search.foreground = "#cad3f5";
    macchiato_theme.colors_search.border = "#8aadf4";
    macchiato_theme.colors_dialog.background = "#24273a";
    macchiato_theme.colors_dialog.foreground = "#cad3f5";
    macchiato_theme.colors_dialog.border = "#8aadf4";
    macchiato_theme.colors_syntax.keyword = "#c6a0f6";
    macchiato_theme.colors_syntax.string = "#a6da95";
    macchiato_theme.colors_syntax.comment = "#5b6078";
    macchiato_theme.colors_syntax.number = "#f5a97f";
    macchiato_theme.colors_syntax.function = "#8aadf4";
    macchiato_theme.colors_syntax.type = "#eed49f";
    macchiato_theme.colors_syntax.operator_ = "#8bd5ca";
    macchiato_theme.colors_syntax.preprocessor = "#ed8796";
    macchiato_theme.colors_syntax.identifier = "#cad3f5";
    macchiato_theme.colors_syntax.punctuation = "#5b6078";
    macchiato_theme.colors_syntax.property = "#8bd5ca";
    macchiato_theme.colors_syntax.tag = "#c6a0f6";
    macchiato_theme.colors_syntax.attribute = "#eed49f";
    macchiato_theme.colors_syntax.regex = "#f5a97f";
    macchiato_theme.colors_syntax.decorator = "#ed8796";
    macchiato_theme.colors_syntax.line_number = "#494d64";
    predefined_themes_["macchiato"] = macchiato_theme;

    // ---- GitHub Dark ----
    FTBConfig github_theme;
    github_theme.colors_main.background = "#0d1117";
    github_theme.colors_main.foreground = "#c9d1d9";
    github_theme.colors_main.border = "#21262d";
    github_theme.colors_main.selection_bg = "#1f6feb";
    github_theme.colors_main.selection_fg = "#ffffff";
    github_theme.colors_files.directory = "#58a6ff";
    github_theme.colors_files.file = "#c9d1d9";
    github_theme.colors_files.executable = "#3fb950";
    github_theme.colors_files.link = "#bc8cff";
    github_theme.colors_files.hidden = "#484f58";
    github_theme.colors_files.system = "#f85149";
    github_theme.colors_status.background = "#161b22";
    github_theme.colors_status.foreground = "#c9d1d9";
    github_theme.colors_status.border = "#21262d";
    github_theme.colors_search.background = "#0d1117";
    github_theme.colors_search.foreground = "#c9d1d9";
    github_theme.colors_search.border = "#58a6ff";
    github_theme.colors_dialog.background = "#0d1117";
    github_theme.colors_dialog.foreground = "#c9d1d9";
    github_theme.colors_dialog.border = "#58a6ff";
    github_theme.colors_syntax.keyword = "#ff7b72";
    github_theme.colors_syntax.string = "#a5d6ff";
    github_theme.colors_syntax.comment = "#484f58";
    github_theme.colors_syntax.number = "#79c0ff";
    github_theme.colors_syntax.function = "#d2a8ff";
    github_theme.colors_syntax.type = "#ffa657";
    github_theme.colors_syntax.operator_ = "#ff7b72";
    github_theme.colors_syntax.preprocessor = "#ff7b72";
    github_theme.colors_syntax.identifier = "#c9d1d9";
    github_theme.colors_syntax.punctuation = "#484f58";
    github_theme.colors_syntax.property = "#79c0ff";
    github_theme.colors_syntax.tag = "#ff7b72";
    github_theme.colors_syntax.attribute = "#ffa657";
    github_theme.colors_syntax.regex = "#a5d6ff";
    github_theme.colors_syntax.decorator = "#d2a8ff";
    github_theme.colors_syntax.line_number = "#21262d";
    predefined_themes_["github"] = github_theme;

    // ---- SynthWave '84 ----
    FTBConfig synthwave_theme;
    synthwave_theme.colors_main.background = "#262335";
    synthwave_theme.colors_main.foreground = "#c0b9dd";
    synthwave_theme.colors_main.border = "#362f4a";
    synthwave_theme.colors_main.selection_bg = "#362f4a";
    synthwave_theme.colors_main.selection_fg = "#c0b9dd";
    synthwave_theme.colors_files.directory = "#36f9f6";
    synthwave_theme.colors_files.file = "#c0b9dd";
    synthwave_theme.colors_files.executable = "#72f1b8";
    synthwave_theme.colors_files.link = "#f92aad";
    synthwave_theme.colors_files.hidden = "#594d6b";
    synthwave_theme.colors_files.system = "#fe4450";
    synthwave_theme.colors_status.background = "#1b1828";
    synthwave_theme.colors_status.foreground = "#c0b9dd";
    synthwave_theme.colors_status.border = "#362f4a";
    synthwave_theme.colors_search.background = "#262335";
    synthwave_theme.colors_search.foreground = "#c0b9dd";
    synthwave_theme.colors_search.border = "#36f9f6";
    synthwave_theme.colors_dialog.background = "#262335";
    synthwave_theme.colors_dialog.foreground = "#c0b9dd";
    synthwave_theme.colors_dialog.border = "#36f9f6";
    synthwave_theme.colors_syntax.keyword = "#f92aad";
    synthwave_theme.colors_syntax.string = "#72f1b8";
    synthwave_theme.colors_syntax.comment = "#594d6b";
    synthwave_theme.colors_syntax.number = "#f97e72";
    synthwave_theme.colors_syntax.function = "#36f9f6";
    synthwave_theme.colors_syntax.type = "#f4eee4";
    synthwave_theme.colors_syntax.operator_ = "#f92aad";
    synthwave_theme.colors_syntax.preprocessor = "#fe4450";
    synthwave_theme.colors_syntax.identifier = "#c0b9dd";
    synthwave_theme.colors_syntax.punctuation = "#594d6b";
    synthwave_theme.colors_syntax.property = "#36f9f6";
    synthwave_theme.colors_syntax.tag = "#f92aad";
    synthwave_theme.colors_syntax.attribute = "#f4eee4";
    synthwave_theme.colors_syntax.regex = "#f97e72";
    synthwave_theme.colors_syntax.decorator = "#fe4450";
    synthwave_theme.colors_syntax.line_number = "#362f4a";
    predefined_themes_["synthwave"] = synthwave_theme;

    // ---- Nightfox ----
    FTBConfig nightfox_theme;
    nightfox_theme.colors_main.background = "#192330";
    nightfox_theme.colors_main.foreground = "#cdcecf";
    nightfox_theme.colors_main.border = "#253244";
    nightfox_theme.colors_main.selection_bg = "#2e3d53";
    nightfox_theme.colors_main.selection_fg = "#cdcecf";
    nightfox_theme.colors_files.directory = "#8ca5d2";
    nightfox_theme.colors_files.file = "#cdcecf";
    nightfox_theme.colors_files.executable = "#aecd91";
    nightfox_theme.colors_files.link = "#cfa6c6";
    nightfox_theme.colors_files.hidden = "#565f75";
    nightfox_theme.colors_files.system = "#c94f6d";
    nightfox_theme.colors_status.background = "#131a24";
    nightfox_theme.colors_status.foreground = "#cdcecf";
    nightfox_theme.colors_status.border = "#253244";
    nightfox_theme.colors_search.background = "#192330";
    nightfox_theme.colors_search.foreground = "#cdcecf";
    nightfox_theme.colors_search.border = "#8ca5d2";
    nightfox_theme.colors_dialog.background = "#192330";
    nightfox_theme.colors_dialog.foreground = "#cdcecf";
    nightfox_theme.colors_dialog.border = "#8ca5d2";
    nightfox_theme.colors_syntax.keyword = "#cfa6c6";
    nightfox_theme.colors_syntax.string = "#aecd91";
    nightfox_theme.colors_syntax.comment = "#565f75";
    nightfox_theme.colors_syntax.number = "#e4aa80";
    nightfox_theme.colors_syntax.function = "#8ca5d2";
    nightfox_theme.colors_syntax.type = "#dbc074";
    nightfox_theme.colors_syntax.operator_ = "#cfa6c6";
    nightfox_theme.colors_syntax.preprocessor = "#c94f6d";
    nightfox_theme.colors_syntax.identifier = "#cdcecf";
    nightfox_theme.colors_syntax.punctuation = "#565f75";
    nightfox_theme.colors_syntax.property = "#8ca5d2";
    nightfox_theme.colors_syntax.tag = "#cfa6c6";
    nightfox_theme.colors_syntax.attribute = "#dbc074";
    nightfox_theme.colors_syntax.regex = "#e4aa80";
    nightfox_theme.colors_syntax.decorator = "#c94f6d";
    nightfox_theme.colors_syntax.line_number = "#2e3d53";
    predefined_themes_["nightfox"] = nightfox_theme;

    // ---- Tokyo Night Storm ----
    FTBConfig tokyo_storm_theme;
    tokyo_storm_theme.colors_main.background = "#24283b";
    tokyo_storm_theme.colors_main.foreground = "#a9b1d6";
    tokyo_storm_theme.colors_main.border = "#1f2335";
    tokyo_storm_theme.colors_main.selection_bg = "#364a82";
    tokyo_storm_theme.colors_main.selection_fg = "#a9b1d6";
    tokyo_storm_theme.colors_files.directory = "#7dcfff";
    tokyo_storm_theme.colors_files.file = "#a9b1d6";
    tokyo_storm_theme.colors_files.executable = "#9ece6a";
    tokyo_storm_theme.colors_files.link = "#bb9af7";
    tokyo_storm_theme.colors_files.hidden = "#565f89";
    tokyo_storm_theme.colors_files.system = "#f7768e";
    tokyo_storm_theme.colors_status.background = "#1f2335";
    tokyo_storm_theme.colors_status.foreground = "#a9b1d6";
    tokyo_storm_theme.colors_status.border = "#1f2335";
    tokyo_storm_theme.colors_search.background = "#24283b";
    tokyo_storm_theme.colors_search.foreground = "#a9b1d6";
    tokyo_storm_theme.colors_search.border = "#7dcfff";
    tokyo_storm_theme.colors_dialog.background = "#24283b";
    tokyo_storm_theme.colors_dialog.foreground = "#a9b1d6";
    tokyo_storm_theme.colors_dialog.border = "#7dcfff";
    tokyo_storm_theme.colors_syntax.keyword = "#bb9af7";
    tokyo_storm_theme.colors_syntax.string = "#9ece6a";
    tokyo_storm_theme.colors_syntax.comment = "#565f89";
    tokyo_storm_theme.colors_syntax.number = "#ff9e64";
    tokyo_storm_theme.colors_syntax.function = "#7aa2f7";
    tokyo_storm_theme.colors_syntax.type = "#e0af68";
    tokyo_storm_theme.colors_syntax.operator_ = "#89ddff";
    tokyo_storm_theme.colors_syntax.preprocessor = "#f7768e";
    tokyo_storm_theme.colors_syntax.identifier = "#a9b1d6";
    tokyo_storm_theme.colors_syntax.punctuation = "#565f89";
    tokyo_storm_theme.colors_syntax.property = "#73daca";
    tokyo_storm_theme.colors_syntax.tag = "#bb9af7";
    tokyo_storm_theme.colors_syntax.attribute = "#e0af68";
    tokyo_storm_theme.colors_syntax.regex = "#ff9e64";
    tokyo_storm_theme.colors_syntax.decorator = "#f7768e";
    tokyo_storm_theme.colors_syntax.line_number = "#3b4261";
    predefined_themes_["tokyo-storm"] = tokyo_storm_theme;

    // ---- Rosé Pine Dawn ----
    FTBConfig rosepine_dawn_theme;
    rosepine_dawn_theme.colors_main.background = "#faf4ed";
    rosepine_dawn_theme.colors_main.foreground = "#575279";
    rosepine_dawn_theme.colors_main.border = "#dfdad9";
    rosepine_dawn_theme.colors_main.selection_bg = "#dfdad9";
    rosepine_dawn_theme.colors_main.selection_fg = "#575279";
    rosepine_dawn_theme.colors_files.directory = "#286983";
    rosepine_dawn_theme.colors_files.file = "#575279";
    rosepine_dawn_theme.colors_files.executable = "#56949f";
    rosepine_dawn_theme.colors_files.link = "#907aa9";
    rosepine_dawn_theme.colors_files.hidden = "#9893a5";
    rosepine_dawn_theme.colors_files.system = "#b4637a";
    rosepine_dawn_theme.colors_status.background = "#f2e9e1";
    rosepine_dawn_theme.colors_status.foreground = "#575279";
    rosepine_dawn_theme.colors_status.border = "#dfdad9";
    rosepine_dawn_theme.colors_search.background = "#faf4ed";
    rosepine_dawn_theme.colors_search.foreground = "#575279";
    rosepine_dawn_theme.colors_search.border = "#286983";
    rosepine_dawn_theme.colors_dialog.background = "#faf4ed";
    rosepine_dawn_theme.colors_dialog.foreground = "#575279";
    rosepine_dawn_theme.colors_dialog.border = "#286983";
    rosepine_dawn_theme.colors_syntax.keyword = "#286983";
    rosepine_dawn_theme.colors_syntax.string = "#56949f";
    rosepine_dawn_theme.colors_syntax.comment = "#9893a5";
    rosepine_dawn_theme.colors_syntax.number = "#d7827e";
    rosepine_dawn_theme.colors_syntax.function = "#907aa9";
    rosepine_dawn_theme.colors_syntax.type = "#ea9d34";
    rosepine_dawn_theme.colors_syntax.operator_ = "#286983";
    rosepine_dawn_theme.colors_syntax.preprocessor = "#b4637a";
    rosepine_dawn_theme.colors_syntax.identifier = "#575279";
    rosepine_dawn_theme.colors_syntax.punctuation = "#9893a5";
    rosepine_dawn_theme.colors_syntax.property = "#56949f";
    rosepine_dawn_theme.colors_syntax.tag = "#286983";
    rosepine_dawn_theme.colors_syntax.attribute = "#ea9d34";
    rosepine_dawn_theme.colors_syntax.regex = "#d7827e";
    rosepine_dawn_theme.colors_syntax.decorator = "#b4637a";
    rosepine_dawn_theme.colors_syntax.line_number = "#dfdad9";
    predefined_themes_["rose-pine-dawn"] = rosepine_dawn_theme;

    // ---- Gruvbox Light ----
    FTBConfig gruvbox_light_theme;
    gruvbox_light_theme.colors_main.background = "#fbf1c7";
    gruvbox_light_theme.colors_main.foreground = "#3c3836";
    gruvbox_light_theme.colors_main.border = "#d5c4a1";
    gruvbox_light_theme.colors_main.selection_bg = "#d5c4a1";
    gruvbox_light_theme.colors_main.selection_fg = "#3c3836";
    gruvbox_light_theme.colors_files.directory = "#076678";
    gruvbox_light_theme.colors_files.file = "#3c3836";
    gruvbox_light_theme.colors_files.executable = "#79740e";
    gruvbox_light_theme.colors_files.link = "#8f3f71";
    gruvbox_light_theme.colors_files.hidden = "#928374";
    gruvbox_light_theme.colors_files.system = "#9d0006";
    gruvbox_light_theme.colors_status.background = "#ebdbb2";
    gruvbox_light_theme.colors_status.foreground = "#3c3836";
    gruvbox_light_theme.colors_status.border = "#d5c4a1";
    gruvbox_light_theme.colors_search.background = "#fbf1c7";
    gruvbox_light_theme.colors_search.foreground = "#3c3836";
    gruvbox_light_theme.colors_search.border = "#076678";
    gruvbox_light_theme.colors_dialog.background = "#fbf1c7";
    gruvbox_light_theme.colors_dialog.foreground = "#3c3836";
    gruvbox_light_theme.colors_dialog.border = "#076678";
    gruvbox_light_theme.colors_syntax.keyword = "#9d0006";
    gruvbox_light_theme.colors_syntax.string = "#79740e";
    gruvbox_light_theme.colors_syntax.comment = "#928374";
    gruvbox_light_theme.colors_syntax.number = "#8f3f71";
    gruvbox_light_theme.colors_syntax.function = "#076678";
    gruvbox_light_theme.colors_syntax.type = "#b57614";
    gruvbox_light_theme.colors_syntax.operator_ = "#9d0006";
    gruvbox_light_theme.colors_syntax.preprocessor = "#9d0006";
    gruvbox_light_theme.colors_syntax.identifier = "#3c3836";
    gruvbox_light_theme.colors_syntax.punctuation = "#928374";
    gruvbox_light_theme.colors_syntax.property = "#076678";
    gruvbox_light_theme.colors_syntax.tag = "#9d0006";
    gruvbox_light_theme.colors_syntax.attribute = "#b57614";
    gruvbox_light_theme.colors_syntax.regex = "#8f3f71";
    gruvbox_light_theme.colors_syntax.decorator = "#8f3f71";
    gruvbox_light_theme.colors_syntax.line_number = "#d5c4a1";
    predefined_themes_["gruvbox-light"] = gruvbox_light_theme;

    // ---- Oxocarbon (IBM Carbon) ----
    FTBConfig oxocarbon_theme;
    oxocarbon_theme.colors_main.background = "#161616";
    oxocarbon_theme.colors_main.foreground = "#f2f4f8";
    oxocarbon_theme.colors_main.border = "#262626";
    oxocarbon_theme.colors_main.selection_bg = "#393939";
    oxocarbon_theme.colors_main.selection_fg = "#f2f4f8";
    oxocarbon_theme.colors_files.directory = "#78a9ff";
    oxocarbon_theme.colors_files.file = "#f2f4f8";
    oxocarbon_theme.colors_files.executable = "#42be65";
    oxocarbon_theme.colors_files.link = "#be95ff";
    oxocarbon_theme.colors_files.hidden = "#525252";
    oxocarbon_theme.colors_files.system = "#fa4d56";
    oxocarbon_theme.colors_status.background = "#0b0b0b";
    oxocarbon_theme.colors_status.foreground = "#f2f4f8";
    oxocarbon_theme.colors_status.border = "#262626";
    oxocarbon_theme.colors_search.background = "#161616";
    oxocarbon_theme.colors_search.foreground = "#f2f4f8";
    oxocarbon_theme.colors_search.border = "#78a9ff";
    oxocarbon_theme.colors_dialog.background = "#161616";
    oxocarbon_theme.colors_dialog.foreground = "#f2f4f8";
    oxocarbon_theme.colors_dialog.border = "#78a9ff";
    oxocarbon_theme.colors_syntax.keyword = "#ff7eb6";
    oxocarbon_theme.colors_syntax.string = "#42be65";
    oxocarbon_theme.colors_syntax.comment = "#525252";
    oxocarbon_theme.colors_syntax.number = "#42be65";
    oxocarbon_theme.colors_syntax.function = "#78a9ff";
    oxocarbon_theme.colors_syntax.type = "#f2f4f8";
    oxocarbon_theme.colors_syntax.operator_ = "#ff7eb6";
    oxocarbon_theme.colors_syntax.preprocessor = "#fa4d56";
    oxocarbon_theme.colors_syntax.identifier = "#f2f4f8";
    oxocarbon_theme.colors_syntax.punctuation = "#525252";
    oxocarbon_theme.colors_syntax.property = "#78a9ff";
    oxocarbon_theme.colors_syntax.tag = "#ff7eb6";
    oxocarbon_theme.colors_syntax.attribute = "#f2f4f8";
    oxocarbon_theme.colors_syntax.regex = "#42be65";
    oxocarbon_theme.colors_syntax.decorator = "#fa4d56";
    oxocarbon_theme.colors_syntax.line_number = "#393939";
    predefined_themes_["oxocarbon"] = oxocarbon_theme;

    // ---- Ayu Mirage ----
    FTBConfig ayu_mirage_theme;
    ayu_mirage_theme.colors_main.background = "#1f2430";
    ayu_mirage_theme.colors_main.foreground = "#cbccc6";
    ayu_mirage_theme.colors_main.border = "#2a3340";
    ayu_mirage_theme.colors_main.selection_bg = "#33415e";
    ayu_mirage_theme.colors_main.selection_fg = "#cbccc6";
    ayu_mirage_theme.colors_files.directory = "#5ccfe6";
    ayu_mirage_theme.colors_files.file = "#cbccc6";
    ayu_mirage_theme.colors_files.executable = "#a6cc70";
    ayu_mirage_theme.colors_files.link = "#d4bfff";
    ayu_mirage_theme.colors_files.hidden = "#565b66";
    ayu_mirage_theme.colors_files.system = "#f27983";
    ayu_mirage_theme.colors_status.background = "#171b24";
    ayu_mirage_theme.colors_status.foreground = "#cbccc6";
    ayu_mirage_theme.colors_status.border = "#2a3340";
    ayu_mirage_theme.colors_search.background = "#1f2430";
    ayu_mirage_theme.colors_search.foreground = "#cbccc6";
    ayu_mirage_theme.colors_search.border = "#5ccfe6";
    ayu_mirage_theme.colors_dialog.background = "#1f2430";
    ayu_mirage_theme.colors_dialog.foreground = "#cbccc6";
    ayu_mirage_theme.colors_dialog.border = "#5ccfe6";
    ayu_mirage_theme.colors_syntax.keyword = "#ffa759";
    ayu_mirage_theme.colors_syntax.string = "#a6cc70";
    ayu_mirage_theme.colors_syntax.comment = "#565b66";
    ayu_mirage_theme.colors_syntax.number = "#f29e74";
    ayu_mirage_theme.colors_syntax.function = "#ffdfb3";
    ayu_mirage_theme.colors_syntax.type = "#73d0ff";
    ayu_mirage_theme.colors_syntax.operator_ = "#f29e74";
    ayu_mirage_theme.colors_syntax.preprocessor = "#f27983";
    ayu_mirage_theme.colors_syntax.identifier = "#cbccc6";
    ayu_mirage_theme.colors_syntax.punctuation = "#565b66";
    ayu_mirage_theme.colors_syntax.property = "#5ccfe6";
    ayu_mirage_theme.colors_syntax.tag = "#5ccfe6";
    ayu_mirage_theme.colors_syntax.attribute = "#ffdfb3";
    ayu_mirage_theme.colors_syntax.regex = "#95e6cb";
    ayu_mirage_theme.colors_syntax.decorator = "#f29e74";
    ayu_mirage_theme.colors_syntax.line_number = "#2a3340";
    predefined_themes_["ayu-mirage"] = ayu_mirage_theme;

    // ---- Everforest Light ----
    FTBConfig everforest_light_theme;
    everforest_light_theme.colors_main.background = "#fdf6e3";
    everforest_light_theme.colors_main.foreground = "#5c6a72";
    everforest_light_theme.colors_main.border = "#d3c6aa";
    everforest_light_theme.colors_main.selection_bg = "#d3c6aa";
    everforest_light_theme.colors_main.selection_fg = "#5c6a72";
    everforest_light_theme.colors_files.directory = "#7fbbb3";
    everforest_light_theme.colors_files.file = "#5c6a72";
    everforest_light_theme.colors_files.executable = "#83c092";
    everforest_light_theme.colors_files.link = "#d699b6";
    everforest_light_theme.colors_files.hidden = "#9da9a0";
    everforest_light_theme.colors_files.system = "#e67e80";
    everforest_light_theme.colors_status.background = "#f0ebd8";
    everforest_light_theme.colors_status.foreground = "#5c6a72";
    everforest_light_theme.colors_status.border = "#d3c6aa";
    everforest_light_theme.colors_search.background = "#fdf6e3";
    everforest_light_theme.colors_search.foreground = "#5c6a72";
    everforest_light_theme.colors_search.border = "#7fbbb3";
    everforest_light_theme.colors_dialog.background = "#fdf6e3";
    everforest_light_theme.colors_dialog.foreground = "#5c6a72";
    everforest_light_theme.colors_dialog.border = "#7fbbb3";
    everforest_light_theme.colors_syntax.keyword = "#e67e80";
    everforest_light_theme.colors_syntax.string = "#83c092";
    everforest_light_theme.colors_syntax.comment = "#9da9a0";
    everforest_light_theme.colors_syntax.number = "#e69875";
    everforest_light_theme.colors_syntax.function = "#7fbbb3";
    everforest_light_theme.colors_syntax.type = "#dbbc7f";
    everforest_light_theme.colors_syntax.operator_ = "#e69875";
    everforest_light_theme.colors_syntax.preprocessor = "#e67e80";
    everforest_light_theme.colors_syntax.identifier = "#5c6a72";
    everforest_light_theme.colors_syntax.punctuation = "#9da9a0";
    everforest_light_theme.colors_syntax.property = "#7fbbb3";
    everforest_light_theme.colors_syntax.tag = "#e67e80";
    everforest_light_theme.colors_syntax.attribute = "#dbbc7f";
    everforest_light_theme.colors_syntax.regex = "#e69875";
    everforest_light_theme.colors_syntax.decorator = "#d699b6";
    everforest_light_theme.colors_syntax.line_number = "#d3c6aa";
    predefined_themes_["everforest-light"] = everforest_light_theme;

    // ---- Andromeda ----
    FTBConfig andromeda_theme;
    andromeda_theme.colors_main.background = "#23262e";
    andromeda_theme.colors_main.foreground = "#e5e5e5";
    andromeda_theme.colors_main.border = "#2f323c";
    andromeda_theme.colors_main.selection_bg = "#2f323c";
    andromeda_theme.colors_main.selection_fg = "#e5e5e5";
    andromeda_theme.colors_files.directory = "#00e8c6";
    andromeda_theme.colors_files.file = "#e5e5e5";
    andromeda_theme.colors_files.executable = "#8be9fd";
    andromeda_theme.colors_files.link = "#ffb8d1";
    andromeda_theme.colors_files.hidden = "#757a8a";
    andromeda_theme.colors_files.system = "#ff5c57";
    andromeda_theme.colors_status.background = "#1c1e24";
    andromeda_theme.colors_status.foreground = "#e5e5e5";
    andromeda_theme.colors_status.border = "#2f323c";
    andromeda_theme.colors_search.background = "#23262e";
    andromeda_theme.colors_search.foreground = "#e5e5e5";
    andromeda_theme.colors_search.border = "#00e8c6";
    andromeda_theme.colors_dialog.background = "#23262e";
    andromeda_theme.colors_dialog.foreground = "#e5e5e5";
    andromeda_theme.colors_dialog.border = "#00e8c6";
    andromeda_theme.colors_syntax.keyword = "#ff5c57";
    andromeda_theme.colors_syntax.string = "#5af78e";
    andromeda_theme.colors_syntax.comment = "#757a8a";
    andromeda_theme.colors_syntax.number = "#ff9f43";
    andromeda_theme.colors_syntax.function = "#57c7ff";
    andromeda_theme.colors_syntax.type = "#ff9f43";
    andromeda_theme.colors_syntax.operator_ = "#ff5c57";
    andromeda_theme.colors_syntax.preprocessor = "#ff5c57";
    andromeda_theme.colors_syntax.identifier = "#e5e5e5";
    andromeda_theme.colors_syntax.punctuation = "#757a8a";
    andromeda_theme.colors_syntax.property = "#57c7ff";
    andromeda_theme.colors_syntax.tag = "#ff5c57";
    andromeda_theme.colors_syntax.attribute = "#ff9f43";
    andromeda_theme.colors_syntax.regex = "#5af78e";
    andromeda_theme.colors_syntax.decorator = "#ffb8d1";
    andromeda_theme.colors_syntax.line_number = "#2f323c";
    predefined_themes_["andromeda"] = andromeda_theme;

    // ---- Catppuccin Frappé ----
    FTBConfig frappe_theme;
    frappe_theme.colors_main.background = "#303446";
    frappe_theme.colors_main.foreground = "#c6d0f5";
    frappe_theme.colors_main.border = "#414559";
    frappe_theme.colors_main.selection_bg = "#51576d";
    frappe_theme.colors_main.selection_fg = "#c6d0f5";
    frappe_theme.colors_files.directory = "#8caaee";
    frappe_theme.colors_files.file = "#c6d0f5";
    frappe_theme.colors_files.executable = "#a6d189";
    frappe_theme.colors_files.link = "#f4b8e4";
    frappe_theme.colors_files.hidden = "#626880";
    frappe_theme.colors_files.system = "#e78284";
    frappe_theme.colors_status.background = "#292c3c";
    frappe_theme.colors_status.foreground = "#c6d0f5";
    frappe_theme.colors_status.border = "#414559";
    frappe_theme.colors_search.background = "#303446";
    frappe_theme.colors_search.foreground = "#c6d0f5";
    frappe_theme.colors_search.border = "#8caaee";
    frappe_theme.colors_dialog.background = "#303446";
    frappe_theme.colors_dialog.foreground = "#c6d0f5";
    frappe_theme.colors_dialog.border = "#8caaee";
    frappe_theme.colors_syntax.keyword = "#ca9ee6";
    frappe_theme.colors_syntax.string = "#a6d189";
    frappe_theme.colors_syntax.comment = "#626880";
    frappe_theme.colors_syntax.number = "#ef9f76";
    frappe_theme.colors_syntax.function = "#8caaee";
    frappe_theme.colors_syntax.type = "#e5c890";
    frappe_theme.colors_syntax.operator_ = "#81c8be";
    frappe_theme.colors_syntax.preprocessor = "#e78284";
    frappe_theme.colors_syntax.identifier = "#c6d0f5";
    frappe_theme.colors_syntax.punctuation = "#626880";
    frappe_theme.colors_syntax.property = "#81c8be";
    frappe_theme.colors_syntax.tag = "#ca9ee6";
    frappe_theme.colors_syntax.attribute = "#e5c890";
    frappe_theme.colors_syntax.regex = "#ef9f76";
    frappe_theme.colors_syntax.decorator = "#e78284";
    frappe_theme.colors_syntax.line_number = "#51576d";
    predefined_themes_["frappe"] = frappe_theme;

    // ---- Doom One ----
    FTBConfig doom_one_theme;
    doom_one_theme.colors_main.background = "#282c34";
    doom_one_theme.colors_main.foreground = "#bbc2cf";
    doom_one_theme.colors_main.border = "#3e4451";
    doom_one_theme.colors_main.selection_bg = "#3e4451";
    doom_one_theme.colors_main.selection_fg = "#bbc2cf";
    doom_one_theme.colors_files.directory = "#51afef";
    doom_one_theme.colors_files.file = "#bbc2cf";
    doom_one_theme.colors_files.executable = "#98be65";
    doom_one_theme.colors_files.link = "#c678dd";
    doom_one_theme.colors_files.hidden = "#5b6268";
    doom_one_theme.colors_files.system = "#ff6c6b";
    doom_one_theme.colors_status.background = "#21242b";
    doom_one_theme.colors_status.foreground = "#bbc2cf";
    doom_one_theme.colors_status.border = "#3e4451";
    doom_one_theme.colors_search.background = "#282c34";
    doom_one_theme.colors_search.foreground = "#bbc2cf";
    doom_one_theme.colors_search.border = "#51afef";
    doom_one_theme.colors_dialog.background = "#282c34";
    doom_one_theme.colors_dialog.foreground = "#bbc2cf";
    doom_one_theme.colors_dialog.border = "#51afef";
    doom_one_theme.colors_syntax.keyword = "#c678dd";
    doom_one_theme.colors_syntax.string = "#98be65";
    doom_one_theme.colors_syntax.comment = "#5b6268";
    doom_one_theme.colors_syntax.number = "#da8548";
    doom_one_theme.colors_syntax.function = "#51afef";
    doom_one_theme.colors_syntax.type = "#ecbe7b";
    doom_one_theme.colors_syntax.operator_ = "#c678dd";
    doom_one_theme.colors_syntax.preprocessor = "#ff6c6b";
    doom_one_theme.colors_syntax.identifier = "#bbc2cf";
    doom_one_theme.colors_syntax.punctuation = "#5b6268";
    doom_one_theme.colors_syntax.property = "#51afef";
    doom_one_theme.colors_syntax.tag = "#c678dd";
    doom_one_theme.colors_syntax.attribute = "#ecbe7b";
    doom_one_theme.colors_syntax.regex = "#98be65";
    doom_one_theme.colors_syntax.decorator = "#ff6c6b";
    doom_one_theme.colors_syntax.line_number = "#3e4451";
    predefined_themes_["doom-one"] = doom_one_theme;

    // ---- Outrun (Retro Neon) ----
    FTBConfig outrun_theme;
    outrun_theme.colors_main.background = "#0b0a1a";
    outrun_theme.colors_main.foreground = "#e0e0e0";
    outrun_theme.colors_main.border = "#1c1b3a";
    outrun_theme.colors_main.selection_bg = "#2d2b55";
    outrun_theme.colors_main.selection_fg = "#e0e0e0";
    outrun_theme.colors_files.directory = "#00ffff";
    outrun_theme.colors_files.file = "#e0e0e0";
    outrun_theme.colors_files.executable = "#5fff87";
    outrun_theme.colors_files.link = "#ff6ac1";
    outrun_theme.colors_files.hidden = "#3a3a6b";
    outrun_theme.colors_files.system = "#ff2a5e";
    outrun_theme.colors_status.background = "#070615";
    outrun_theme.colors_status.foreground = "#e0e0e0";
    outrun_theme.colors_status.border = "#1c1b3a";
    outrun_theme.colors_search.background = "#0b0a1a";
    outrun_theme.colors_search.foreground = "#e0e0e0";
    outrun_theme.colors_search.border = "#00ffff";
    outrun_theme.colors_dialog.background = "#0b0a1a";
    outrun_theme.colors_dialog.foreground = "#e0e0e0";
    outrun_theme.colors_dialog.border = "#00ffff";
    outrun_theme.colors_syntax.keyword = "#ff6ac1";
    outrun_theme.colors_syntax.string = "#5fff87";
    outrun_theme.colors_syntax.comment = "#3a3a6b";
    outrun_theme.colors_syntax.number = "#ff9f43";
    outrun_theme.colors_syntax.function = "#00ffff";
    outrun_theme.colors_syntax.type = "#ffb86c";
    outrun_theme.colors_syntax.operator_ = "#ff6ac1";
    outrun_theme.colors_syntax.preprocessor = "#ff2a5e";
    outrun_theme.colors_syntax.identifier = "#e0e0e0";
    outrun_theme.colors_syntax.punctuation = "#3a3a6b";
    outrun_theme.colors_syntax.property = "#5fff87";
    outrun_theme.colors_syntax.tag = "#ff6ac1";
    outrun_theme.colors_syntax.attribute = "#ffb86c";
    outrun_theme.colors_syntax.regex = "#ff9f43";
    outrun_theme.colors_syntax.decorator = "#ff2a5e";
    outrun_theme.colors_syntax.line_number = "#1c1b3a";
    predefined_themes_["outrun"] = outrun_theme;

    // ---- Falcon (Purple Dark) ----
    FTBConfig falcon_theme;
    falcon_theme.colors_main.background = "#1e1e2e";
    falcon_theme.colors_main.foreground = "#cdd6f4";
    falcon_theme.colors_main.border = "#2e2e3e";
    falcon_theme.colors_main.selection_bg = "#3e3e5e";
    falcon_theme.colors_main.selection_fg = "#cdd6f4";
    falcon_theme.colors_files.directory = "#b4befe";
    falcon_theme.colors_files.file = "#cdd6f4";
    falcon_theme.colors_files.executable = "#81c8be";
    falcon_theme.colors_files.link = "#f5c2e7";
    falcon_theme.colors_files.hidden = "#585b70";
    falcon_theme.colors_files.system = "#f38ba8";
    falcon_theme.colors_status.background = "#181825";
    falcon_theme.colors_status.foreground = "#cdd6f4";
    falcon_theme.colors_status.border = "#2e2e3e";
    falcon_theme.colors_search.background = "#1e1e2e";
    falcon_theme.colors_search.foreground = "#cdd6f4";
    falcon_theme.colors_search.border = "#b4befe";
    falcon_theme.colors_dialog.background = "#1e1e2e";
    falcon_theme.colors_dialog.foreground = "#cdd6f4";
    falcon_theme.colors_dialog.border = "#b4befe";
    falcon_theme.colors_syntax.keyword = "#cba6f7";
    falcon_theme.colors_syntax.string = "#81c8be";
    falcon_theme.colors_syntax.comment = "#585b70";
    falcon_theme.colors_syntax.number = "#fab387";
    falcon_theme.colors_syntax.function = "#b4befe";
    falcon_theme.colors_syntax.type = "#f9e2af";
    falcon_theme.colors_syntax.operator_ = "#cba6f7";
    falcon_theme.colors_syntax.preprocessor = "#f38ba8";
    falcon_theme.colors_syntax.identifier = "#cdd6f4";
    falcon_theme.colors_syntax.punctuation = "#585b70";
    falcon_theme.colors_syntax.property = "#81c8be";
    falcon_theme.colors_syntax.tag = "#cba6f7";
    falcon_theme.colors_syntax.attribute = "#f9e2af";
    falcon_theme.colors_syntax.regex = "#fab387";
    falcon_theme.colors_syntax.decorator = "#f38ba8";
    falcon_theme.colors_syntax.line_number = "#2e2e3e";
    predefined_themes_["falcon"] = falcon_theme;

    // ---- VSCode Dark+ ----
    FTBConfig vscode_theme;
    vscode_theme.colors_main.background = "#1e1e1e";
    vscode_theme.colors_main.foreground = "#d4d4d4";
    vscode_theme.colors_main.border = "#252526";
    vscode_theme.colors_main.selection_bg = "#264f78";
    vscode_theme.colors_main.selection_fg = "#d4d4d4";
    vscode_theme.colors_files.directory = "#4ec9b0";
    vscode_theme.colors_files.file = "#d4d4d4";
    vscode_theme.colors_files.executable = "#6a9955";
    vscode_theme.colors_files.link = "#c586c0";
    vscode_theme.colors_files.hidden = "#5a5a5a";
    vscode_theme.colors_files.system = "#f44747";
    vscode_theme.colors_status.background = "#141414";
    vscode_theme.colors_status.foreground = "#d4d4d4";
    vscode_theme.colors_status.border = "#252526";
    vscode_theme.colors_search.background = "#1e1e1e";
    vscode_theme.colors_search.foreground = "#d4d4d4";
    vscode_theme.colors_search.border = "#4ec9b0";
    vscode_theme.colors_dialog.background = "#1e1e1e";
    vscode_theme.colors_dialog.foreground = "#d4d4d4";
    vscode_theme.colors_dialog.border = "#4ec9b0";
    vscode_theme.colors_syntax.keyword = "#569cd6";
    vscode_theme.colors_syntax.string = "#ce9178";
    vscode_theme.colors_syntax.comment = "#6a9955";
    vscode_theme.colors_syntax.number = "#b5cea8";
    vscode_theme.colors_syntax.function = "#dcdcaa";
    vscode_theme.colors_syntax.type = "#4ec9b0";
    vscode_theme.colors_syntax.operator_ = "#d4d4d4";
    vscode_theme.colors_syntax.preprocessor = "#c586c0";
    vscode_theme.colors_syntax.identifier = "#d4d4d4";
    vscode_theme.colors_syntax.punctuation = "#5a5a5a";
    vscode_theme.colors_syntax.property = "#dcdcaa";
    vscode_theme.colors_syntax.tag = "#569cd6";
    vscode_theme.colors_syntax.attribute = "#9cdcfe";
    vscode_theme.colors_syntax.regex = "#d16969";
    vscode_theme.colors_syntax.decorator = "#c586c0";
    vscode_theme.colors_syntax.line_number = "#2d2d2d";
    predefined_themes_["vscode"] = vscode_theme;

    // ---- Moonfly ----
    FTBConfig moonfly_theme;
    moonfly_theme.colors_main.background = "#080808";
    moonfly_theme.colors_main.foreground = "#bdbdbd";
    moonfly_theme.colors_main.border = "#1c1c1c";
    moonfly_theme.colors_main.selection_bg = "#2a2a2a";
    moonfly_theme.colors_main.selection_fg = "#bdbdbd";
    moonfly_theme.colors_files.directory = "#80a0ff";
    moonfly_theme.colors_files.file = "#bdbdbd";
    moonfly_theme.colors_files.executable = "#8cc85f";
    moonfly_theme.colors_files.link = "#ff5189";
    moonfly_theme.colors_files.hidden = "#444444";
    moonfly_theme.colors_files.system = "#ff5454";
    moonfly_theme.colors_status.background = "#111111";
    moonfly_theme.colors_status.foreground = "#bdbdbd";
    moonfly_theme.colors_status.border = "#1c1c1c";
    moonfly_theme.colors_search.background = "#080808";
    moonfly_theme.colors_search.foreground = "#bdbdbd";
    moonfly_theme.colors_search.border = "#80a0ff";
    moonfly_theme.colors_dialog.background = "#080808";
    moonfly_theme.colors_dialog.foreground = "#bdbdbd";
    moonfly_theme.colors_dialog.border = "#80a0ff";
    moonfly_theme.colors_syntax.keyword = "#ff5189";
    moonfly_theme.colors_syntax.string = "#8cc85f";
    moonfly_theme.colors_syntax.comment = "#444444";
    moonfly_theme.colors_syntax.number = "#ffb267";
    moonfly_theme.colors_syntax.function = "#80a0ff";
    moonfly_theme.colors_syntax.type = "#e3c78a";
    moonfly_theme.colors_syntax.operator_ = "#ff5189";
    moonfly_theme.colors_syntax.preprocessor = "#ff5454";
    moonfly_theme.colors_syntax.identifier = "#bdbdbd";
    moonfly_theme.colors_syntax.punctuation = "#444444";
    moonfly_theme.colors_syntax.property = "#80a0ff";
    moonfly_theme.colors_syntax.tag = "#ff5189";
    moonfly_theme.colors_syntax.attribute = "#e3c78a";
    moonfly_theme.colors_syntax.regex = "#ffb267";
    moonfly_theme.colors_syntax.decorator = "#ff5454";
    moonfly_theme.colors_syntax.line_number = "#1c1c1c";
    predefined_themes_["moonfly"] = moonfly_theme;

    // ---- Tomorrow Night ----
    FTBConfig tomorrow_night_theme;
    tomorrow_night_theme.colors_main.background = "#1d1f21";
    tomorrow_night_theme.colors_main.foreground = "#c5c8c6";
    tomorrow_night_theme.colors_main.border = "#282a2e";
    tomorrow_night_theme.colors_main.selection_bg = "#373b41";
    tomorrow_night_theme.colors_main.selection_fg = "#c5c8c6";
    tomorrow_night_theme.colors_files.directory = "#81a2be";
    tomorrow_night_theme.colors_files.file = "#c5c8c6";
    tomorrow_night_theme.colors_files.executable = "#b5bd68";
    tomorrow_night_theme.colors_files.link = "#b294bb";
    tomorrow_night_theme.colors_files.hidden = "#5f6368";
    tomorrow_night_theme.colors_files.system = "#cc6666";
    tomorrow_night_theme.colors_status.background = "#151617";
    tomorrow_night_theme.colors_status.foreground = "#c5c8c6";
    tomorrow_night_theme.colors_status.border = "#282a2e";
    tomorrow_night_theme.colors_search.background = "#1d1f21";
    tomorrow_night_theme.colors_search.foreground = "#c5c8c6";
    tomorrow_night_theme.colors_search.border = "#81a2be";
    tomorrow_night_theme.colors_dialog.background = "#1d1f21";
    tomorrow_night_theme.colors_dialog.foreground = "#c5c8c6";
    tomorrow_night_theme.colors_dialog.border = "#81a2be";
    tomorrow_night_theme.colors_syntax.keyword = "#cc6666";
    tomorrow_night_theme.colors_syntax.string = "#b5bd68";
    tomorrow_night_theme.colors_syntax.comment = "#5f6368";
    tomorrow_night_theme.colors_syntax.number = "#de935f";
    tomorrow_night_theme.colors_syntax.function = "#81a2be";
    tomorrow_night_theme.colors_syntax.type = "#f0c674";
    tomorrow_night_theme.colors_syntax.operator_ = "#cc6666";
    tomorrow_night_theme.colors_syntax.preprocessor = "#cc6666";
    tomorrow_night_theme.colors_syntax.identifier = "#c5c8c6";
    tomorrow_night_theme.colors_syntax.punctuation = "#5f6368";
    tomorrow_night_theme.colors_syntax.property = "#81a2be";
    tomorrow_night_theme.colors_syntax.tag = "#cc6666";
    tomorrow_night_theme.colors_syntax.attribute = "#f0c674";
    tomorrow_night_theme.colors_syntax.regex = "#de935f";
    tomorrow_night_theme.colors_syntax.decorator = "#b294bb";
    tomorrow_night_theme.colors_syntax.line_number = "#282a2e";
    predefined_themes_["tomorrow-night"] = tomorrow_night_theme;

    // ---- Zenburn ----
    FTBConfig zenburn_theme;
    zenburn_theme.colors_main.background = "#383838";
    zenburn_theme.colors_main.foreground = "#dcdccc";
    zenburn_theme.colors_main.border = "#2a2a2a";
    zenburn_theme.colors_main.selection_bg = "#4a4a4a";
    zenburn_theme.colors_main.selection_fg = "#dcdccc";
    zenburn_theme.colors_files.directory = "#8fa5b5";
    zenburn_theme.colors_files.file = "#dcdccc";
    zenburn_theme.colors_files.executable = "#7f9f7f";
    zenburn_theme.colors_files.link = "#cc9393";
    zenburn_theme.colors_files.hidden = "#6f6f6f";
    zenburn_theme.colors_files.system = "#f07050";
    zenburn_theme.colors_status.background = "#2a2a2a";
    zenburn_theme.colors_status.foreground = "#dcdccc";
    zenburn_theme.colors_status.border = "#2a2a2a";
    zenburn_theme.colors_search.background = "#383838";
    zenburn_theme.colors_search.foreground = "#dcdccc";
    zenburn_theme.colors_search.border = "#8fa5b5";
    zenburn_theme.colors_dialog.background = "#383838";
    zenburn_theme.colors_dialog.foreground = "#dcdccc";
    zenburn_theme.colors_dialog.border = "#8fa5b5";
    zenburn_theme.colors_syntax.keyword = "#f0dfaf";
    zenburn_theme.colors_syntax.string = "#cc9393";
    zenburn_theme.colors_syntax.comment = "#6f6f6f";
    zenburn_theme.colors_syntax.number = "#dfaf8f";
    zenburn_theme.colors_syntax.function = "#8fa5b5";
    zenburn_theme.colors_syntax.type = "#f0dfaf";
    zenburn_theme.colors_syntax.operator_ = "#dcdccc";
    zenburn_theme.colors_syntax.preprocessor = "#f07050";
    zenburn_theme.colors_syntax.identifier = "#dcdccc";
    zenburn_theme.colors_syntax.punctuation = "#6f6f6f";
    zenburn_theme.colors_syntax.property = "#8fa5b5";
    zenburn_theme.colors_syntax.tag = "#f0dfaf";
    zenburn_theme.colors_syntax.attribute = "#dfaf8f";
    zenburn_theme.colors_syntax.regex = "#cc9393";
    zenburn_theme.colors_syntax.decorator = "#f07050";
    zenburn_theme.colors_syntax.line_number = "#2a2a2a";
    predefined_themes_["zenburn"] = zenburn_theme;

    // ---- Catppuccin Latte (Light) ----
    FTBConfig latte_theme;
    latte_theme.colors_main.background = "#eff1f5";
    latte_theme.colors_main.foreground = "#4c4f69";
    latte_theme.colors_main.border = "#dce0e8";
    latte_theme.colors_main.selection_bg = "#ccd0da";
    latte_theme.colors_main.selection_fg = "#4c4f69";
    latte_theme.colors_files.directory = "#1e66f5";
    latte_theme.colors_files.file = "#4c4f69";
    latte_theme.colors_files.executable = "#40a02b";
    latte_theme.colors_files.link = "#ea76cb";
    latte_theme.colors_files.hidden = "#9ca0b0";
    latte_theme.colors_files.system = "#d20f39";
    latte_theme.colors_status.background = "#e6e9ef";
    latte_theme.colors_status.foreground = "#4c4f69";
    latte_theme.colors_status.border = "#dce0e8";
    latte_theme.colors_search.background = "#eff1f5";
    latte_theme.colors_search.foreground = "#4c4f69";
    latte_theme.colors_search.border = "#1e66f5";
    latte_theme.colors_dialog.background = "#eff1f5";
    latte_theme.colors_dialog.foreground = "#4c4f69";
    latte_theme.colors_dialog.border = "#1e66f5";
    latte_theme.colors_syntax.keyword = "#8839ef";
    latte_theme.colors_syntax.string = "#40a02b";
    latte_theme.colors_syntax.comment = "#9ca0b0";
    latte_theme.colors_syntax.number = "#fe640b";
    latte_theme.colors_syntax.function = "#1e66f5";
    latte_theme.colors_syntax.type = "#df8e1d";
    latte_theme.colors_syntax.operator_ = "#04a5e5";
    latte_theme.colors_syntax.preprocessor = "#d20f39";
    latte_theme.colors_syntax.identifier = "#4c4f69";
    latte_theme.colors_syntax.punctuation = "#9ca0b0";
    latte_theme.colors_syntax.property = "#04a5e5";
    latte_theme.colors_syntax.tag = "#8839ef";
    latte_theme.colors_syntax.attribute = "#df8e1d";
    latte_theme.colors_syntax.regex = "#fe640b";
    latte_theme.colors_syntax.decorator = "#d20f39";
    latte_theme.colors_syntax.line_number = "#ccd0da";
    predefined_themes_["latte"] = latte_theme;

    // ---- Palenight (Material) ----
    FTBConfig palenight_theme;
    palenight_theme.colors_main.background = "#292d3e";
    palenight_theme.colors_main.foreground = "#babed8";
    palenight_theme.colors_main.border = "#343849";
    palenight_theme.colors_main.selection_bg = "#3d4153";
    palenight_theme.colors_main.selection_fg = "#babed8";
    palenight_theme.colors_files.directory = "#82aaff";
    palenight_theme.colors_files.file = "#babed8";
    palenight_theme.colors_files.executable = "#c3e88d";
    palenight_theme.colors_files.link = "#c792ea";
    palenight_theme.colors_files.hidden = "#606580";
    palenight_theme.colors_files.system = "#f07178";
    palenight_theme.colors_status.background = "#1b1d2b";
    palenight_theme.colors_status.foreground = "#babed8";
    palenight_theme.colors_status.border = "#343849";
    palenight_theme.colors_search.background = "#292d3e";
    palenight_theme.colors_search.foreground = "#babed8";
    palenight_theme.colors_search.border = "#82aaff";
    palenight_theme.colors_dialog.background = "#292d3e";
    palenight_theme.colors_dialog.foreground = "#babed8";
    palenight_theme.colors_dialog.border = "#82aaff";
    palenight_theme.colors_syntax.keyword = "#c792ea";
    palenight_theme.colors_syntax.string = "#c3e88d";
    palenight_theme.colors_syntax.comment = "#606580";
    palenight_theme.colors_syntax.number = "#f78c6c";
    palenight_theme.colors_syntax.function = "#82aaff";
    palenight_theme.colors_syntax.type = "#ffcb6b";
    palenight_theme.colors_syntax.operator_ = "#89ddff";
    palenight_theme.colors_syntax.preprocessor = "#f07178";
    palenight_theme.colors_syntax.identifier = "#babed8";
    palenight_theme.colors_syntax.punctuation = "#606580";
    palenight_theme.colors_syntax.property = "#f07178";
    palenight_theme.colors_syntax.tag = "#c792ea";
    palenight_theme.colors_syntax.attribute = "#ffcb6b";
    palenight_theme.colors_syntax.regex = "#f78c6c";
    palenight_theme.colors_syntax.decorator = "#f07178";
    palenight_theme.colors_syntax.line_number = "#343849";
    predefined_themes_["palenight"] = palenight_theme;

    // ---- Monokai Pro ----
    FTBConfig monokai_pro_theme;
    monokai_pro_theme.colors_main.background = "#2d2a2e";
    monokai_pro_theme.colors_main.foreground = "#fcfcfa";
    monokai_pro_theme.colors_main.border = "#3d3a3e";
    monokai_pro_theme.colors_main.selection_bg = "#4d4a4e";
    monokai_pro_theme.colors_main.selection_fg = "#fcfcfa";
    monokai_pro_theme.colors_files.directory = "#78dce8";
    monokai_pro_theme.colors_files.file = "#fcfcfa";
    monokai_pro_theme.colors_files.executable = "#a9dc76";
    monokai_pro_theme.colors_files.link = "#ab9df2";
    monokai_pro_theme.colors_files.hidden = "#6e6a72";
    monokai_pro_theme.colors_files.system = "#ff6188";
    monokai_pro_theme.colors_status.background = "#1f1d21";
    monokai_pro_theme.colors_status.foreground = "#fcfcfa";
    monokai_pro_theme.colors_status.border = "#3d3a3e";
    monokai_pro_theme.colors_search.background = "#2d2a2e";
    monokai_pro_theme.colors_search.foreground = "#fcfcfa";
    monokai_pro_theme.colors_search.border = "#78dce8";
    monokai_pro_theme.colors_dialog.background = "#2d2a2e";
    monokai_pro_theme.colors_dialog.foreground = "#fcfcfa";
    monokai_pro_theme.colors_dialog.border = "#78dce8";
    monokai_pro_theme.colors_syntax.keyword = "#ff6188";
    monokai_pro_theme.colors_syntax.string = "#a9dc76";
    monokai_pro_theme.colors_syntax.comment = "#6e6a72";
    monokai_pro_theme.colors_syntax.number = "#fc9867";
    monokai_pro_theme.colors_syntax.function = "#78dce8";
    monokai_pro_theme.colors_syntax.type = "#ffd866";
    monokai_pro_theme.colors_syntax.operator_ = "#ab9df2";
    monokai_pro_theme.colors_syntax.preprocessor = "#ff6188";
    monokai_pro_theme.colors_syntax.identifier = "#fcfcfa";
    monokai_pro_theme.colors_syntax.punctuation = "#6e6a72";
    monokai_pro_theme.colors_syntax.property = "#78dce8";
    monokai_pro_theme.colors_syntax.tag = "#ff6188";
    monokai_pro_theme.colors_syntax.attribute = "#ffd866";
    monokai_pro_theme.colors_syntax.regex = "#fc9867";
    monokai_pro_theme.colors_syntax.decorator = "#ab9df2";
    monokai_pro_theme.colors_syntax.line_number = "#3d3a3e";
    predefined_themes_["monokai-pro"] = monokai_pro_theme;

    // ---- Flexoki (Warm Ink) ----
    FTBConfig flexoki_theme;
    flexoki_theme.colors_main.background = "#1b1813";
    flexoki_theme.colors_main.foreground = "#dad8ce";
    flexoki_theme.colors_main.border = "#2e2a25";
    flexoki_theme.colors_main.selection_bg = "#3e3a35";
    flexoki_theme.colors_main.selection_fg = "#dad8ce";
    flexoki_theme.colors_files.directory = "#76afb6";
    flexoki_theme.colors_files.file = "#dad8ce";
    flexoki_theme.colors_files.executable = "#8bb86a";
    flexoki_theme.colors_files.link = "#ce96c8";
    flexoki_theme.colors_files.hidden = "#6f6e69";
    flexoki_theme.colors_files.system = "#d14c41";
    flexoki_theme.colors_status.background = "#100f0c";
    flexoki_theme.colors_status.foreground = "#dad8ce";
    flexoki_theme.colors_status.border = "#2e2a25";
    flexoki_theme.colors_search.background = "#1b1813";
    flexoki_theme.colors_search.foreground = "#dad8ce";
    flexoki_theme.colors_search.border = "#76afb6";
    flexoki_theme.colors_dialog.background = "#1b1813";
    flexoki_theme.colors_dialog.foreground = "#dad8ce";
    flexoki_theme.colors_dialog.border = "#76afb6";
    flexoki_theme.colors_syntax.keyword = "#d14c41";
    flexoki_theme.colors_syntax.string = "#8bb86a";
    flexoki_theme.colors_syntax.comment = "#6f6e69";
    flexoki_theme.colors_syntax.number = "#d4834a";
    flexoki_theme.colors_syntax.function = "#76afb6";
    flexoki_theme.colors_syntax.type = "#d0a34e";
    flexoki_theme.colors_syntax.operator_ = "#d14c41";
    flexoki_theme.colors_syntax.preprocessor = "#d14c41";
    flexoki_theme.colors_syntax.identifier = "#dad8ce";
    flexoki_theme.colors_syntax.punctuation = "#6f6e69";
    flexoki_theme.colors_syntax.property = "#76afb6";
    flexoki_theme.colors_syntax.tag = "#d14c41";
    flexoki_theme.colors_syntax.attribute = "#d0a34e";
    flexoki_theme.colors_syntax.regex = "#d4834a";
    flexoki_theme.colors_syntax.decorator = "#ce96c8";
    flexoki_theme.colors_syntax.line_number = "#2e2a25";
    predefined_themes_["flexoki"] = flexoki_theme;

    // ---- One Light Pro ----
    FTBConfig one_light_theme;
    one_light_theme.colors_main.background = "#fafafa";
    one_light_theme.colors_main.foreground = "#383a42";
    one_light_theme.colors_main.border = "#e0e0e0";
    one_light_theme.colors_main.selection_bg = "#e5e5e5";
    one_light_theme.colors_main.selection_fg = "#383a42";
    one_light_theme.colors_files.directory = "#4078f2";
    one_light_theme.colors_files.file = "#383a42";
    one_light_theme.colors_files.executable = "#50a14f";
    one_light_theme.colors_files.link = "#a626a4";
    one_light_theme.colors_files.hidden = "#a0a1a7";
    one_light_theme.colors_files.system = "#e45649";
    one_light_theme.colors_status.background = "#ececec";
    one_light_theme.colors_status.foreground = "#383a42";
    one_light_theme.colors_status.border = "#e0e0e0";
    one_light_theme.colors_search.background = "#fafafa";
    one_light_theme.colors_search.foreground = "#383a42";
    one_light_theme.colors_search.border = "#4078f2";
    one_light_theme.colors_dialog.background = "#fafafa";
    one_light_theme.colors_dialog.foreground = "#383a42";
    one_light_theme.colors_dialog.border = "#4078f2";
    one_light_theme.colors_syntax.keyword = "#a626a4";
    one_light_theme.colors_syntax.string = "#50a14f";
    one_light_theme.colors_syntax.comment = "#a0a1a7";
    one_light_theme.colors_syntax.number = "#986801";
    one_light_theme.colors_syntax.function = "#4078f2";
    one_light_theme.colors_syntax.type = "#c18401";
    one_light_theme.colors_syntax.operator_ = "#0184bc";
    one_light_theme.colors_syntax.preprocessor = "#e45649";
    one_light_theme.colors_syntax.identifier = "#383a42";
    one_light_theme.colors_syntax.punctuation = "#a0a1a7";
    one_light_theme.colors_syntax.property = "#4078f2";
    one_light_theme.colors_syntax.tag = "#e45649";
    one_light_theme.colors_syntax.attribute = "#c18401";
    one_light_theme.colors_syntax.regex = "#50a14f";
    one_light_theme.colors_syntax.decorator = "#986801";
    one_light_theme.colors_syntax.line_number = "#e0e0e0";
    predefined_themes_["one-light"] = one_light_theme;

    // ---- Nord Light ----
    FTBConfig nord_light_theme;
    nord_light_theme.colors_main.background = "#f0f4f8";
    nord_light_theme.colors_main.foreground = "#4c566a";
    nord_light_theme.colors_main.border = "#d8dee9";
    nord_light_theme.colors_main.selection_bg = "#d8dee9";
    nord_light_theme.colors_main.selection_fg = "#4c566a";
    nord_light_theme.colors_files.directory = "#5e81ac";
    nord_light_theme.colors_files.file = "#4c566a";
    nord_light_theme.colors_files.executable = "#a3be8c";
    nord_light_theme.colors_files.link = "#b48ead";
    nord_light_theme.colors_files.hidden = "#8fbcbb";
    nord_light_theme.colors_files.system = "#bf616a";
    nord_light_theme.colors_status.background = "#e5e9f0";
    nord_light_theme.colors_status.foreground = "#4c566a";
    nord_light_theme.colors_status.border = "#d8dee9";
    nord_light_theme.colors_search.background = "#f0f4f8";
    nord_light_theme.colors_search.foreground = "#4c566a";
    nord_light_theme.colors_search.border = "#5e81ac";
    nord_light_theme.colors_dialog.background = "#f0f4f8";
    nord_light_theme.colors_dialog.foreground = "#4c566a";
    nord_light_theme.colors_dialog.border = "#5e81ac";
    nord_light_theme.colors_syntax.keyword = "#b48ead";
    nord_light_theme.colors_syntax.string = "#a3be8c";
    nord_light_theme.colors_syntax.comment = "#8fbcbb";
    nord_light_theme.colors_syntax.number = "#d08770";
    nord_light_theme.colors_syntax.function = "#5e81ac";
    nord_light_theme.colors_syntax.type = "#ebcb8b";
    nord_light_theme.colors_syntax.operator_ = "#5e81ac";
    nord_light_theme.colors_syntax.preprocessor = "#bf616a";
    nord_light_theme.colors_syntax.identifier = "#4c566a";
    nord_light_theme.colors_syntax.punctuation = "#8fbcbb";
    nord_light_theme.colors_syntax.property = "#5e81ac";
    nord_light_theme.colors_syntax.tag = "#b48ead";
    nord_light_theme.colors_syntax.attribute = "#ebcb8b";
    nord_light_theme.colors_syntax.regex = "#d08770";
    nord_light_theme.colors_syntax.decorator = "#bf616a";
    nord_light_theme.colors_syntax.line_number = "#d8dee9";
    predefined_themes_["nord-light"] = nord_light_theme;

    // ---- Challenger Deep ----
    FTBConfig challenger_theme;
    challenger_theme.colors_main.background = "#1e1e3e";
    challenger_theme.colors_main.foreground = "#cbe3e7";
    challenger_theme.colors_main.border = "#3e3e5e";
    challenger_theme.colors_main.selection_bg = "#5a5a7e";
    challenger_theme.colors_main.selection_fg = "#cbe3e7";
    challenger_theme.colors_files.directory = "#82e2ff";
    challenger_theme.colors_files.file = "#cbe3e7";
    challenger_theme.colors_files.executable = "#a1cdbf";
    challenger_theme.colors_files.link = "#ffcb6b";
    challenger_theme.colors_files.hidden = "#5654a4";
    challenger_theme.colors_files.system = "#f28779";
    challenger_theme.colors_status.background = "#2c2e4e";
    challenger_theme.colors_status.foreground = "#cbe3e7";
    challenger_theme.colors_status.border = "#3e3e5e";
    challenger_theme.colors_search.background = "#1e1e3e";
    challenger_theme.colors_search.foreground = "#cbe3e7";
    challenger_theme.colors_search.border = "#82e2ff";
    challenger_theme.colors_dialog.background = "#1e1e3e";
    challenger_theme.colors_dialog.foreground = "#cbe3e7";
    challenger_theme.colors_dialog.border = "#82e2ff";
    challenger_theme.colors_syntax.keyword = "#c991e1";
    challenger_theme.colors_syntax.string = "#a1cdbf";
    challenger_theme.colors_syntax.comment = "#5654a4";
    challenger_theme.colors_syntax.number = "#f09483";
    challenger_theme.colors_syntax.function = "#82e2ff";
    challenger_theme.colors_syntax.type = "#ffcb6b";
    challenger_theme.colors_syntax.operator_ = "#c991e1";
    challenger_theme.colors_syntax.preprocessor = "#f28779";
    challenger_theme.colors_syntax.identifier = "#cbe3e7";
    challenger_theme.colors_syntax.punctuation = "#5654a4";
    challenger_theme.colors_syntax.property = "#82e2ff";
    challenger_theme.colors_syntax.tag = "#c991e1";
    challenger_theme.colors_syntax.attribute = "#ffcb6b";
    challenger_theme.colors_syntax.regex = "#f09483";
    challenger_theme.colors_syntax.decorator = "#f28779";
    challenger_theme.colors_syntax.line_number = "#3e3e5e";
    predefined_themes_["challenger"] = challenger_theme;

    // ---- Cobalt2 ----
    FTBConfig cobalt2_theme;
    cobalt2_theme.colors_main.background = "#132738";
    cobalt2_theme.colors_main.foreground = "#ffffff";
    cobalt2_theme.colors_main.border = "#1c3a52";
    cobalt2_theme.colors_main.selection_bg = "#0d4f8b";
    cobalt2_theme.colors_main.selection_fg = "#ffffff";
    cobalt2_theme.colors_files.directory = "#ffc600";
    cobalt2_theme.colors_files.file = "#ffffff";
    cobalt2_theme.colors_files.executable = "#7fec8e";
    cobalt2_theme.colors_files.link = "#ff7edb";
    cobalt2_theme.colors_files.hidden = "#556677";
    cobalt2_theme.colors_files.system = "#ec5c61";
    cobalt2_theme.colors_status.background = "#0b2942";
    cobalt2_theme.colors_status.foreground = "#ffffff";
    cobalt2_theme.colors_status.border = "#1c3a52";
    cobalt2_theme.colors_search.background = "#132738";
    cobalt2_theme.colors_search.foreground = "#ffffff";
    cobalt2_theme.colors_search.border = "#ffc600";
    cobalt2_theme.colors_dialog.background = "#132738";
    cobalt2_theme.colors_dialog.foreground = "#ffffff";
    cobalt2_theme.colors_dialog.border = "#ffc600";
    cobalt2_theme.colors_syntax.keyword = "#ff9d00";
    cobalt2_theme.colors_syntax.string = "#7fec8e";
    cobalt2_theme.colors_syntax.comment = "#556677";
    cobalt2_theme.colors_syntax.number = "#ff628c";
    cobalt2_theme.colors_syntax.function = "#ffc600";
    cobalt2_theme.colors_syntax.type = "#ffcb6b";
    cobalt2_theme.colors_syntax.operator_ = "#ff9d00";
    cobalt2_theme.colors_syntax.preprocessor = "#ec5c61";
    cobalt2_theme.colors_syntax.identifier = "#ffffff";
    cobalt2_theme.colors_syntax.punctuation = "#556677";
    cobalt2_theme.colors_syntax.property = "#ffc600";
    cobalt2_theme.colors_syntax.tag = "#ff9d00";
    cobalt2_theme.colors_syntax.attribute = "#ffcb6b";
    cobalt2_theme.colors_syntax.regex = "#ff628c";
    cobalt2_theme.colors_syntax.decorator = "#ec5c61";
    cobalt2_theme.colors_syntax.line_number = "#1c3a52";
    predefined_themes_["cobalt2"] = cobalt2_theme;

    // ---- Iceberg ----
    FTBConfig iceberg_theme;
    iceberg_theme.colors_main.background = "#161821";
    iceberg_theme.colors_main.foreground = "#c6c8d1";
    iceberg_theme.colors_main.border = "#2e303e";
    iceberg_theme.colors_main.selection_bg = "#3e4460";
    iceberg_theme.colors_main.selection_fg = "#c6c8d1";
    iceberg_theme.colors_files.directory = "#668ecc";
    iceberg_theme.colors_files.file = "#c6c8d1";
    iceberg_theme.colors_files.executable = "#b4b8c5";
    iceberg_theme.colors_files.link = "#a099c4";
    iceberg_theme.colors_files.hidden = "#444b5e";
    iceberg_theme.colors_files.system = "#e27878";
    iceberg_theme.colors_status.background = "#0e1015";
    iceberg_theme.colors_status.foreground = "#c6c8d1";
    iceberg_theme.colors_status.border = "#2e303e";
    iceberg_theme.colors_search.background = "#161821";
    iceberg_theme.colors_search.foreground = "#c6c8d1";
    iceberg_theme.colors_search.border = "#668ecc";
    iceberg_theme.colors_dialog.background = "#161821";
    iceberg_theme.colors_dialog.foreground = "#c6c8d1";
    iceberg_theme.colors_dialog.border = "#668ecc";
    iceberg_theme.colors_syntax.keyword = "#e27878";
    iceberg_theme.colors_syntax.string = "#b4b8c5";
    iceberg_theme.colors_syntax.comment = "#444b5e";
    iceberg_theme.colors_syntax.number = "#e27878";
    iceberg_theme.colors_syntax.function = "#668ecc";
    iceberg_theme.colors_syntax.type = "#e2a478";
    iceberg_theme.colors_syntax.operator_ = "#e27878";
    iceberg_theme.colors_syntax.preprocessor = "#e27878";
    iceberg_theme.colors_syntax.identifier = "#c6c8d1";
    iceberg_theme.colors_syntax.punctuation = "#444b5e";
    iceberg_theme.colors_syntax.property = "#668ecc";
    iceberg_theme.colors_syntax.tag = "#e27878";
    iceberg_theme.colors_syntax.attribute = "#e2a478";
    iceberg_theme.colors_syntax.regex = "#e2a478";
    iceberg_theme.colors_syntax.decorator = "#e27878";
    iceberg_theme.colors_syntax.line_number = "#2e303e";
    predefined_themes_["iceberg"] = iceberg_theme;

    // ---- Wombat ----
    FTBConfig wombat_theme;
    wombat_theme.colors_main.background = "#141219";
    wombat_theme.colors_main.foreground = "#cdc9c3";
    wombat_theme.colors_main.border = "#2d2b2f";
    wombat_theme.colors_main.selection_bg = "#3d3b3f";
    wombat_theme.colors_main.selection_fg = "#cdc9c3";
    wombat_theme.colors_files.directory = "#dca571";
    wombat_theme.colors_files.file = "#cdc9c3";
    wombat_theme.colors_files.executable = "#93e0e3";
    wombat_theme.colors_files.link = "#c991e1";
    wombat_theme.colors_files.hidden = "#5e5c60";
    wombat_theme.colors_files.system = "#dca571";
    wombat_theme.colors_status.background = "#0e0c0f";
    wombat_theme.colors_status.foreground = "#cdc9c3";
    wombat_theme.colors_status.border = "#2d2b2f";
    wombat_theme.colors_search.background = "#141219";
    wombat_theme.colors_search.foreground = "#cdc9c3";
    wombat_theme.colors_search.border = "#dca571";
    wombat_theme.colors_dialog.background = "#141219";
    wombat_theme.colors_dialog.foreground = "#cdc9c3";
    wombat_theme.colors_dialog.border = "#dca571";
    wombat_theme.colors_syntax.keyword = "#dca571";
    wombat_theme.colors_syntax.string = "#93e0e3";
    wombat_theme.colors_syntax.comment = "#5e5c60";
    wombat_theme.colors_syntax.number = "#dca571";
    wombat_theme.colors_syntax.function = "#dca571";
    wombat_theme.colors_syntax.type = "#e69f78";
    wombat_theme.colors_syntax.operator_ = "#dca571";
    wombat_theme.colors_syntax.preprocessor = "#dca571";
    wombat_theme.colors_syntax.identifier = "#cdc9c3";
    wombat_theme.colors_syntax.punctuation = "#5e5c60";
    wombat_theme.colors_syntax.property = "#dca571";
    wombat_theme.colors_syntax.tag = "#dca571";
    wombat_theme.colors_syntax.attribute = "#e69f78";
    wombat_theme.colors_syntax.regex = "#e69f78";
    wombat_theme.colors_syntax.decorator = "#dca571";
    wombat_theme.colors_syntax.line_number = "#2d2b2f";
    predefined_themes_["wombat"] = wombat_theme;

    // ---- Shades of Purple ----
    FTBConfig shades_purple_theme;
    shades_purple_theme.colors_main.background = "#1e1d4b";
    shades_purple_theme.colors_main.foreground = "#a599e9";
    shades_purple_theme.colors_main.border = "#2d2c5e";
    shades_purple_theme.colors_main.selection_bg = "#3d3c7e";
    shades_purple_theme.colors_main.selection_fg = "#fad000";
    shades_purple_theme.colors_files.directory = "#00e5ff";
    shades_purple_theme.colors_files.file = "#a599e9";
    shades_purple_theme.colors_files.executable = "#00e676";
    shades_purple_theme.colors_files.link = "#ff6ac1";
    shades_purple_theme.colors_files.hidden = "#5b5999";
    shades_purple_theme.colors_files.system = "#f55036";
    shades_purple_theme.colors_status.background = "#181744";
    shades_purple_theme.colors_status.foreground = "#a599e9";
    shades_purple_theme.colors_status.border = "#2d2c5e";
    shades_purple_theme.colors_search.background = "#1e1d4b";
    shades_purple_theme.colors_search.foreground = "#a599e9";
    shades_purple_theme.colors_search.border = "#fad000";
    shades_purple_theme.colors_dialog.background = "#1e1d4b";
    shades_purple_theme.colors_dialog.foreground = "#a599e9";
    shades_purple_theme.colors_dialog.border = "#fad000";
    shades_purple_theme.colors_syntax.keyword = "#ff6ac1";
    shades_purple_theme.colors_syntax.string = "#00e676";
    shades_purple_theme.colors_syntax.comment = "#5b5999";
    shades_purple_theme.colors_syntax.number = "#fad000";
    shades_purple_theme.colors_syntax.function = "#00e5ff";
    shades_purple_theme.colors_syntax.type = "#ff9ac1";
    shades_purple_theme.colors_syntax.operator_ = "#ff6ac1";
    shades_purple_theme.colors_syntax.preprocessor = "#f55036";
    shades_purple_theme.colors_syntax.identifier = "#a599e9";
    shades_purple_theme.colors_syntax.punctuation = "#5b5999";
    shades_purple_theme.colors_syntax.property = "#00e5ff";
    shades_purple_theme.colors_syntax.tag = "#ff6ac1";
    shades_purple_theme.colors_syntax.attribute = "#fad000";
    shades_purple_theme.colors_syntax.regex = "#fad000";
    shades_purple_theme.colors_syntax.decorator = "#f55036";
    shades_purple_theme.colors_syntax.line_number = "#2d2c5e";
    predefined_themes_["shades-of-purple"] = shades_purple_theme;

    // ---- Tokyo Light ----
    FTBConfig tokyo_light_theme;
    tokyo_light_theme.colors_main.background = "#e8e5d9";
    tokyo_light_theme.colors_main.foreground = "#3760bf";
    tokyo_light_theme.colors_main.border = "#d5d1c6";
    tokyo_light_theme.colors_main.selection_bg = "#c9c5b5";
    tokyo_light_theme.colors_main.selection_fg = "#3760bf";
    tokyo_light_theme.colors_files.directory = "#2e7de9";
    tokyo_light_theme.colors_files.file = "#3760bf";
    tokyo_light_theme.colors_files.executable = "#587539";
    tokyo_light_theme.colors_files.link = "#8c6dc3";
    tokyo_light_theme.colors_files.hidden = "#9699a3";
    tokyo_light_theme.colors_files.system = "#f52a65";
    tokyo_light_theme.colors_status.background = "#d5d1c6";
    tokyo_light_theme.colors_status.foreground = "#3760bf";
    tokyo_light_theme.colors_status.border = "#d5d1c6";
    tokyo_light_theme.colors_search.background = "#e8e5d9";
    tokyo_light_theme.colors_search.foreground = "#3760bf";
    tokyo_light_theme.colors_search.border = "#2e7de9";
    tokyo_light_theme.colors_dialog.background = "#e8e5d9";
    tokyo_light_theme.colors_dialog.foreground = "#3760bf";
    tokyo_light_theme.colors_dialog.border = "#2e7de9";
    tokyo_light_theme.colors_syntax.keyword = "#8c6dc3";
    tokyo_light_theme.colors_syntax.string = "#587539";
    tokyo_light_theme.colors_syntax.comment = "#9699a3";
    tokyo_light_theme.colors_syntax.number = "#c64343";
    tokyo_light_theme.colors_syntax.function = "#2e7de9";
    tokyo_light_theme.colors_syntax.type = "#9854f1";
    tokyo_light_theme.colors_syntax.operator_ = "#3760bf";
    tokyo_light_theme.colors_syntax.preprocessor = "#f52a65";
    tokyo_light_theme.colors_syntax.identifier = "#3760bf";
    tokyo_light_theme.colors_syntax.punctuation = "#9699a3";
    tokyo_light_theme.colors_syntax.property = "#2e7de9";
    tokyo_light_theme.colors_syntax.tag = "#8c6dc3";
    tokyo_light_theme.colors_syntax.attribute = "#9854f1";
    tokyo_light_theme.colors_syntax.regex = "#c64343";
    tokyo_light_theme.colors_syntax.decorator = "#f52a65";
    tokyo_light_theme.colors_syntax.line_number = "#d5d1c6";
    predefined_themes_["tokyo-light"] = tokyo_light_theme;

    // ---- Papercolor Dark ----
    FTBConfig papercolor_dark_theme;
    papercolor_dark_theme.colors_main.background = "#1c1c1c";
    papercolor_dark_theme.colors_main.foreground = "#bcbcbc";
    papercolor_dark_theme.colors_main.border = "#444444";
    papercolor_dark_theme.colors_main.selection_bg = "#444444";
    papercolor_dark_theme.colors_main.selection_fg = "#d0d0d0";
    papercolor_dark_theme.colors_files.directory = "#5fafaf";
    papercolor_dark_theme.colors_files.file = "#d0d0d0";
    papercolor_dark_theme.colors_files.executable = "#87d75f";
    papercolor_dark_theme.colors_files.link = "#d787af";
    papercolor_dark_theme.colors_files.hidden = "#767676";
    papercolor_dark_theme.colors_files.system = "#d75f5f";
    papercolor_dark_theme.colors_status.background = "#303030";
    papercolor_dark_theme.colors_status.foreground = "#d0d0d0";
    papercolor_dark_theme.colors_status.border = "#444444";
    papercolor_dark_theme.colors_search.background = "#1c1c1c";
    papercolor_dark_theme.colors_search.foreground = "#d0d0d0";
    papercolor_dark_theme.colors_search.border = "#5fafaf";
    papercolor_dark_theme.colors_dialog.background = "#1c1c1c";
    papercolor_dark_theme.colors_dialog.foreground = "#d0d0d0";
    papercolor_dark_theme.colors_dialog.border = "#5fafaf";
    papercolor_dark_theme.colors_syntax.keyword = "#ffaf87";
    papercolor_dark_theme.colors_syntax.string = "#87d75f";
    papercolor_dark_theme.colors_syntax.comment = "#767676";
    papercolor_dark_theme.colors_syntax.number = "#d75faf";
    papercolor_dark_theme.colors_syntax.function = "#5fafaf";
    papercolor_dark_theme.colors_syntax.type = "#d7af87";
    papercolor_dark_theme.colors_syntax.operator_ = "#ffaf87";
    papercolor_dark_theme.colors_syntax.preprocessor = "#d75f5f";
    papercolor_dark_theme.colors_syntax.identifier = "#d0d0d0";
    papercolor_dark_theme.colors_syntax.punctuation = "#767676";
    papercolor_dark_theme.colors_syntax.property = "#5fafaf";
    papercolor_dark_theme.colors_syntax.tag = "#ffaf87";
    papercolor_dark_theme.colors_syntax.attribute = "#d7af87";
    papercolor_dark_theme.colors_syntax.regex = "#d75faf";
    papercolor_dark_theme.colors_syntax.decorator = "#d75f5f";
    papercolor_dark_theme.colors_syntax.line_number = "#444444";
    predefined_themes_["papercolor-dark"] = papercolor_dark_theme;
}

void ThemeManager::ApplyThemeConfig(const std::string& theme_name) {
    auto it = predefined_themes_.find(theme_name);
    if (it != predefined_themes_.end()) {
        theme_config_ = it->second;
    } else {
        // 如果主题不存在，使用默认主题
        theme_config_ = predefined_themes_["default"];
        std::cerr << "Warning: theme '" << theme_name << "' not found, using default" << std::endl;
    }
}

void ThemeManager::CreateThemeColorMap(const std::string& /*theme_name*/) {
    current_theme_colors_.clear();

    // 使用 ConfigManager 的 ParseColor 来解析十六进制颜色
    auto* cm = ConfigManager::GetInstance();

    // 从 theme_config_ 动态读取颜色
    const auto& tc = theme_config_;

    // 主界面颜色
    current_theme_colors_["main_bg"]      = cm->ParseColor(tc.colors_main.background);
    current_theme_colors_["main_fg"]      = cm->ParseColor(tc.colors_main.foreground);
    current_theme_colors_["main_border"]  = cm->ParseColor(tc.colors_main.border);
    current_theme_colors_["selection_bg"] = cm->ParseColor(tc.colors_main.selection_bg);
    current_theme_colors_["selection_fg"] = cm->ParseColor(tc.colors_main.selection_fg);

    // 文件类型颜色
    current_theme_colors_["directory"]  = cm->ParseColor(tc.colors_files.directory);
    current_theme_colors_["file"]       = cm->ParseColor(tc.colors_files.file);
    current_theme_colors_["executable"] = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["link"]       = cm->ParseColor(tc.colors_files.link);
    current_theme_colors_["hidden"]     = cm->ParseColor(tc.colors_files.hidden);
    current_theme_colors_["system"]     = cm->ParseColor(tc.colors_files.system);

    // 状态栏颜色
    current_theme_colors_["status_bg"] = cm->ParseColor(tc.colors_status.background);
    current_theme_colors_["status_fg"] = cm->ParseColor(tc.colors_status.foreground);
    current_theme_colors_["time"]      = cm->ParseColor(tc.colors_files.executable);  // accent
    current_theme_colors_["position"]  = cm->ParseColor(tc.colors_dialog.border);     // accent
    current_theme_colors_["path"]      = cm->ParseColor(tc.colors_files.directory);   // accent

    // 搜索框颜色
    current_theme_colors_["search_bg"]        = cm->ParseColor(tc.colors_search.background);
    current_theme_colors_["search_fg"]        = cm->ParseColor(tc.colors_search.foreground);
    current_theme_colors_["search_border"]    = cm->ParseColor(tc.colors_search.border);
    current_theme_colors_["search_highlight"] = cm->ParseColor(tc.colors_files.executable);  // accent

    // 对话框颜色
    current_theme_colors_["dialog_bg"]     = cm->ParseColor(tc.colors_dialog.background);
    current_theme_colors_["dialog_fg"]     = cm->ParseColor(tc.colors_dialog.foreground);
    current_theme_colors_["dialog_border"] = cm->ParseColor(tc.colors_dialog.border);
    current_theme_colors_["button_bg"]     = cm->ParseColor(tc.colors_search.border);   // accent
    current_theme_colors_["button_fg"]     = cm->ParseColor(tc.colors_main.background);
    current_theme_colors_["input_bg"]      = cm->ParseColor(tc.colors_status.background);
    current_theme_colors_["input_fg"]      = cm->ParseColor(tc.colors_main.foreground);

    // UI 元素颜色 (派生自主色)
    current_theme_colors_["tab_active_bg"]   = cm->ParseColor(tc.colors_search.border);
    current_theme_colors_["tab_active_fg"]   = cm->ParseColor(tc.colors_main.background);
    current_theme_colors_["tab_inactive_fg"] = cm->ParseColor(tc.colors_files.hidden);
    current_theme_colors_["indicator"]       = cm->ParseColor(tc.colors_search.border);
    current_theme_colors_["marker_copied"]   = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["marker_cut"]      = cm->ParseColor(tc.colors_files.system);
    current_theme_colors_["marker_selected"] = cm->ParseColor(tc.colors_search.border);
    current_theme_colors_["preview_border"]  = cm->ParseColor(tc.colors_main.border);
    current_theme_colors_["separator"]       = cm->ParseColor(tc.colors_status.background);
    current_theme_colors_["hovered_bg"]      = cm->ParseColor(tc.colors_main.border);
    current_theme_colors_["find_keyword"]    = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["gauge_fill"]      = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["gauge_bg"]        = cm->ParseColor(tc.colors_status.background);
    current_theme_colors_["title"]           = cm->ParseColor(tc.colors_dialog.border);
    current_theme_colors_["error"]           = cm->ParseColor(tc.colors_files.system);
    current_theme_colors_["success"]         = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["warning"]         = cm->ParseColor(tc.colors_files.executable);
    current_theme_colors_["dim"]             = cm->ParseColor(tc.colors_files.hidden);

    // Powerline 专用背景色
    current_theme_colors_["find_keyword_bg"] = cm->ParseColor(tc.colors_search.border);
    current_theme_colors_["success_bg"]      = cm->ParseColor(tc.colors_main.border);

    // 语法高亮颜色 (从 colors_syntax 读取)
    current_theme_colors_["syn_keyword"]      = cm->ParseColor(tc.colors_syntax.keyword);
    current_theme_colors_["syn_string"]       = cm->ParseColor(tc.colors_syntax.string);
    current_theme_colors_["syn_comment"]      = cm->ParseColor(tc.colors_syntax.comment);
    current_theme_colors_["syn_number"]       = cm->ParseColor(tc.colors_syntax.number);
    current_theme_colors_["syn_function"]     = cm->ParseColor(tc.colors_syntax.function);
    current_theme_colors_["syn_type"]         = cm->ParseColor(tc.colors_syntax.type);
    current_theme_colors_["syn_operator"]     = cm->ParseColor(tc.colors_syntax.operator_);
    current_theme_colors_["syn_preprocessor"] = cm->ParseColor(tc.colors_syntax.preprocessor);
    current_theme_colors_["syn_identifier"]   = cm->ParseColor(tc.colors_syntax.identifier);
    current_theme_colors_["syn_punctuation"]  = cm->ParseColor(tc.colors_syntax.punctuation);
    current_theme_colors_["syn_property"]     = cm->ParseColor(tc.colors_syntax.property);
    current_theme_colors_["syn_tag"]          = cm->ParseColor(tc.colors_syntax.tag);
    current_theme_colors_["syn_attribute"]    = cm->ParseColor(tc.colors_syntax.attribute);
    current_theme_colors_["syn_regex"]        = cm->ParseColor(tc.colors_syntax.regex);
    current_theme_colors_["syn_decorator"]    = cm->ParseColor(tc.colors_syntax.decorator);
    current_theme_colors_["syn_line_number"]  = cm->ParseColor(tc.colors_syntax.line_number);
}

} // namespace FTB 