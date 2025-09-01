#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <map>
#include <memory>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/color.hpp>

namespace FTB {

// 颜色配置结构
struct ColorConfig {
    std::string background;
    std::string foreground;
    std::string border;
    std::string selection_bg;
    std::string selection_fg;
    
    ColorConfig() : background("black"), foreground("white"), 
                   border("blue"), selection_bg("blue"), selection_fg("white") {}
};

// 文件类型颜色配置
struct FileTypeColors {
    std::string directory;
    std::string file;
    std::string executable;
    std::string link;
    std::string hidden;
    std::string system;
    
    FileTypeColors() : directory("blue"), file("white"), executable("green"),
                      link("cyan"), hidden("yellow"), system("red") {}
};

// 界面样式配置
struct StyleConfig {
    bool show_icons;
    bool show_file_size;
    bool show_modified_time;
    bool show_permissions;
    bool enable_mouse;
    bool enable_animations;
    
    StyleConfig() : show_icons(true), show_file_size(true), 
                   show_modified_time(true), show_permissions(true),
                   enable_mouse(true), enable_animations(true) {}
};

// 布局配置
struct LayoutConfig {
    int items_per_page;
    int items_per_row;
    double detail_panel_ratio;
    bool show_detail_panel;
    
    LayoutConfig() : items_per_page(20), items_per_row(5), 
                    detail_panel_ratio(0.3), show_detail_panel(true) {}
};

// 刷新配置
struct RefreshConfig {
    int ui_refresh_interval;
    int content_refresh_interval;
    bool auto_refresh;
    
    RefreshConfig() : ui_refresh_interval(100), content_refresh_interval(1000), 
                     auto_refresh(true) {}
};

// 主题配置
struct ThemeConfig {
    std::string name;
    bool use_colors;
    bool use_bold;
    bool use_underline;
    
    ThemeConfig() : name("default"), use_colors(true), 
                   use_bold(false), use_underline(false) {}
};

// MySQL配置
struct MySQLConfig {
    std::string default_host;
    int default_port;
    std::string default_username;
    std::string default_database;
    int connection_timeout;
    
    MySQLConfig() : default_host("localhost"), default_port(3306), 
                   default_username("root"), default_database(""), 
                   connection_timeout(10) {}
};

// SSH配置
struct SSHConfig {
    int default_port;
    int connection_timeout;
    bool save_connection_history;
    int max_connection_history;
    
    SSHConfig() : default_port(22), connection_timeout(30), 
                 save_connection_history(true), max_connection_history(10) {}
};

// 日志配置
struct LoggingConfig {
    std::string level;
    bool output_to_file;
    std::string log_file;
    bool show_timestamp;
    
    LoggingConfig() : level("info"), output_to_file(false), 
                     log_file("~/.ftb.log"), show_timestamp(true) {}
};

// 主配置结构
struct FTBConfig {
    ColorConfig colors_main;
    FileTypeColors colors_files;
    ColorConfig colors_status;
    ColorConfig colors_search;
    ColorConfig colors_dialog;
    StyleConfig style;
    LayoutConfig layout;
    RefreshConfig refresh;
    ThemeConfig theme;
    MySQLConfig mysql;
    SSHConfig ssh;
    LoggingConfig logging;
    
    // 自定义颜色映射
    std::map<std::string, ftxui::Color> custom_colors;
};

// 配置管理器类
class ConfigManager {
public:
    static ConfigManager* GetInstance();
    
    // 加载配置文件
    bool LoadConfig(const std::string& config_path = "");
    
    // 保存配置文件
    bool SaveConfig(const std::string& config_path = "");
    
    // 获取配置
    const FTBConfig& GetConfig() const { return config_; }
    
    // 获取颜色
    ftxui::Color GetColor(const std::string& color_name) const;
    
    // 获取文件类型颜色
    ftxui::Color GetFileTypeColor(const std::string& file_type) const;
    
    // 应用主题
    void ApplyTheme(const std::string& theme_name);
    
    // 重新加载配置
    bool ReloadConfig();
    
    // 获取配置文件路径
    std::string GetConfigPath() const { return config_path_; }
    
    // 检查配置是否有效
    bool IsConfigValid() const { return config_loaded_; }
    
    // 获取默认配置
    FTBConfig GetDefaultConfig() const;
    
    // 重置为默认配置
    void ResetToDefault();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // 解析配置文件
    bool ParseConfigFile(const std::string& content);
    
    // 解析颜色值
    ftxui::Color ParseColor(const std::string& color_str) const;
    
    // 应用颜色配置
    void ApplyColorConfig();
    
    // 创建默认配置文件
    bool CreateDefaultConfig();
    
    // 获取用户主目录
    std::string GetUserHomeDir() const;
    
    // 验证配置值
    bool ValidateConfig() const;

private:
    static ConfigManager* instance_;
    FTBConfig config_;
    std::string config_path_;
    bool config_loaded_;
    
    // 预定义颜色映射
    std::map<std::string, ftxui::Color> predefined_colors_;
    
    // 初始化预定义颜色
    void InitializePredefinedColors();
};

} // namespace FTB

#endif // CONFIG_MANAGER_HPP
