#ifndef THEME_MANAGER_HPP
#define THEME_MANAGER_HPP

#include <string>
#include <map>
#include <memory>
#include <ftxui/dom/elements.hpp>
#include "FTB/ConfigManager.hpp"

namespace FTB {

// 主题管理器类
class ThemeManager {
public:
    static ThemeManager* GetInstance();
    
    // 应用主题
    void ApplyTheme(const std::string& theme_name);
    
    // 获取当前主题名称
    std::string GetCurrentTheme() const { return current_theme_; }
    
    // 获取可用主题列表
    std::vector<std::string> GetAvailableThemes() const;
    
    // 获取主题颜色
    ftxui::Color GetThemeColor(const std::string& color_name) const;
    
    // 获取文件类型颜色
    ftxui::Color GetFileTypeColor(const std::string& file_type) const;
    
    // 应用颜色到元素
    ftxui::Element ApplyColorToElement(ftxui::Element element, const std::string& color_name) const;
    
    // 创建带颜色的文本
    ftxui::Element CreateColoredText(const std::string& text, const std::string& color_name) const;
    
    // 创建带颜色的边框
    ftxui::Element CreateColoredBorder(ftxui::Element element, const std::string& color_name) const;
    
    // 创建带颜色的背景
    ftxui::Element CreateColoredBackground(ftxui::Element element, const std::string& color_name) const;
    
    // 创建选中项样式
    ftxui::Element CreateSelectionStyle(ftxui::Element element) const;
    
    // 创建状态栏样式
    ftxui::Element CreateStatusBarStyle(ftxui::Element element) const;
    
    // 创建搜索框样式
    ftxui::Element CreateSearchBoxStyle(ftxui::Element element) const;
    
    // 创建对话框样式
    ftxui::Element CreateDialogStyle(ftxui::Element element) const;
    
    // 创建按钮样式
    ftxui::Element CreateButtonStyle(ftxui::Element element) const;
    
    // 创建输入框样式
    ftxui::Element CreateInputStyle(ftxui::Element element) const;
    
    // 重新加载主题
    void ReloadTheme();
    
    // 获取主题配置
    const FTBConfig& GetThemeConfig() const;

public:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

private:
    
    // 初始化预定义主题
    void InitializePredefinedThemes();
    
    // 应用主题配置
    void ApplyThemeConfig(const std::string& theme_name);
    
    // 创建主题特定的颜色映射
    void CreateThemeColorMap(const std::string& theme_name);

private:
    static std::unique_ptr<ThemeManager> instance_;
    std::string current_theme_;
    FTBConfig theme_config_;
    
    // 预定义主题配置
    std::map<std::string, FTBConfig> predefined_themes_;
    
    // 当前主题的颜色映射
    std::map<std::string, ftxui::Color> current_theme_colors_;
};

} // namespace FTB

#endif // THEME_MANAGER_HPP 