#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <ftxui/dom/elements.hpp>
#include "ops/OpenerManager.hpp"

namespace FTB {

// ---- 颜色配置 ----
struct ColorConfig {
    std::string background;
    std::string foreground;
    std::string border;
    std::string selection_bg;
    std::string selection_fg;

    ColorConfig() : background("#1e1e2e"), foreground("#cdd6f4"),
                   border("#45475a"), selection_bg("#585b70"), selection_fg("#cdd6f4") {}
};

// ---- 文件类型颜色配置 ----
struct FileTypeColors {
    std::string directory;
    std::string file;
    std::string executable;
    std::string link;
    std::string hidden;
    std::string system;

    FileTypeColors() : directory("#89b4fa"), file("#cdd6f4"), executable("#a6e3a1"),
                      link("#f5c2e7"), hidden("#6c7086"), system("#f38ba8") {}
};

// ---- 语法高亮颜色配置 ----
struct SyntaxColors {
    std::string keyword;       // 关键字
    std::string string;        // 字符串
    std::string comment;       // 注释
    std::string number;        // 数字
    std::string function;      // 函数名
    std::string type;          // 类型
    std::string operator_;     // 操作符
    std::string preprocessor;  // 预处理指令
    std::string identifier;    // 标识符
    std::string punctuation;   // 标点符号
    std::string property;      // 属性/字段
    std::string tag;           // HTML/XML 标签
    std::string attribute;     // 属性名
    std::string regex;         // 正则表达式
    std::string decorator;     // 装饰器
    std::string line_number;   // 行号

    SyntaxColors()
        : keyword("#cba6f7"), string("#a6e3a1"), comment("#6c7086"),
          number("#fab387"), function("#89b4fa"), type("#f9e2af"),
          operator_("#89dceb"), preprocessor("#f38ba8"), identifier("#cdd6f4"),
          punctuation("#6c7086"), property("#89b4fa"), tag("#cba6f7"),
          attribute("#f9e2af"), regex("#fab387"), decorator("#f38ba8"),
          line_number("#585b70") {}
};

// ---- 界面样式配置 ----
struct StyleConfig {
    bool show_icons;
    bool show_file_size;
    bool show_modified_time;
    bool show_permissions;
    bool enable_mouse;
    bool enable_animations;
    bool show_hidden_files;
    bool show_detail_panel;

    std::string sort_mode;

    StyleConfig() : show_icons(true), show_file_size(true),
                   show_modified_time(true), show_permissions(true),
                   enable_mouse(true), enable_animations(false),
                   show_hidden_files(false), show_detail_panel(true),
                   sort_mode("name_asc") {}
};

// ---- 布局配置 ----
struct LayoutConfig {
    double parent_ratio;      // 左栏(父目录)占比 0.0~1.0
    double current_ratio;     // 中栏(当前目录)占比 0.0~1.0
    double preview_ratio;     // 右栏(预览)占比 0.0~1.0
    int items_per_page;       // 0 = auto (根据终端高度)

    LayoutConfig() : parent_ratio(2.0/9.0),    // 22.2% 左栏(父目录)
                    current_ratio(4.0/9.0),   // 44.4% 中栏(当前目录)
                    preview_ratio(3.0/9.0),   // 33.3% 右栏(预览)
                    items_per_page(0) {}

    // 归一化比例，确保总和为1
    void Normalize() {
        double total = parent_ratio + current_ratio + preview_ratio;
        if (total <= 0) { parent_ratio = 2.0/9.0; current_ratio = 4.0/9.0; preview_ratio = 3.0/9.0; total = 1.0; }
        parent_ratio  /= total;
        current_ratio /= total;
        preview_ratio /= total;
    }
};

// ---- 刷新配置 ----
struct RefreshConfig {
    int ui_refresh_interval_ms;
    int content_refresh_interval_ms;
    bool auto_refresh;

    RefreshConfig() : ui_refresh_interval_ms(100), content_refresh_interval_ms(1000),
                     auto_refresh(true) {}
};

// ---- 主题配置 ----
struct ThemeConfig {
    std::string name;
    bool use_bold;
    bool use_underline;

    ThemeConfig() : name("default"), use_bold(false), use_underline(false) {}
};

// ---- SSH配置 ----
struct SSHConfig {
    int default_port;
    int connection_timeout;
    bool save_connection_history;
    int max_connection_history;

    SSHConfig() : default_port(22), connection_timeout(30),
                 save_connection_history(true), max_connection_history(10) {}
};

// ---- 日志配置 ----
struct LoggingConfig {
    std::string level;
    bool output_to_file;
    std::string log_file;
    bool show_timestamp;

    LoggingConfig() : level("info"), output_to_file(false),
                     log_file("~/.config/ftb/ftb.log"), show_timestamp(true) {}
};

// ---- SSH 连接记录 ----
struct SSHRecord {
    std::string host;
    int port = 22;
    std::string username;
    std::string remote_directory = "/home";
};

// ---- 书签配置 ----
struct BookmarkConfig {
    std::map<std::string, std::string> bookmarks;  // name -> path

    BookmarkConfig() {
        bookmarks["home"] = "~";
        bookmarks["root"] = "/";
        bookmarks["tmp"]  = "/tmp";
    }
};

// ---- UI 装饰风格配置 ----
struct UIConfig {
    std::string column_separator = "thin";     // thin | light | heavy | double | dotted | dashed | none
    std::string panel_border     = "rounded";   // rounded | sharp | double | heavy | none
    std::string selection_style  = "full";      // full | underline | invert | bar | minimal
    std::string tab_bar_style    = "modern";    // modern | classic

    UIConfig() = default;
};

// ---- 状态栏风格配置 ----
struct StatusBarConfig {
    std::string style = "powerline";  // powerline | rounded | thin | arrow | bar | minimal
    bool show_position = true;        // 显示 选中项/总数
    bool show_time     = true;        // 显示当前时间
    bool show_clipboard = true;       // 显示剪贴板待粘贴项数
    bool use_bold      = false;       // 加粗

    StatusBarConfig() = default;
};

// ---- 预览配置 ----
struct PreviewConfig {
    int max_text_file_size_kb = 0;      // 0 = no limit
    int max_text_lines = 0;             // 0 = no limit
    int max_dir_entries = 0;            // 0 = no limit
    int max_hex_bytes = 0;              // 0 = no limit
    int max_archive_nodes = 0;          // 0 = no limit
    int max_spreadsheet_rows = 0;       // 0 = no limit
    bool truncate_long_lines = true;    // whether to truncate lines exceeding panel width
    int chunk_size_lines = 200;         // lines per chunk for lazy text loading
    int virtual_scroll_margin = 50;     // extra lines to preload above/below viewport

    PreviewConfig() = default;
};

// ---- 主配置结构 ----
struct FTBConfig {
    ColorConfig colors_main;
    FileTypeColors colors_files;
    SyntaxColors colors_syntax;
    ColorConfig colors_status;
    ColorConfig colors_search;
    ColorConfig colors_dialog;
    StyleConfig style;
    LayoutConfig layout;
    UIConfig ui;
    StatusBarConfig statusbar;
    RefreshConfig refresh;
    ThemeConfig theme;
    SSHConfig ssh;
    LoggingConfig logging;
    BookmarkConfig bookmarks;
    OpenerConfig opener;
    PreviewConfig preview;

    // 自定义颜色映射
    std::map<std::string, ftxui::Color> custom_colors;
};

// ---- 配置管理器 ----
class ConfigManager {
public:
    static ConfigManager* GetInstance();

    bool LoadConfig(const std::string& config_path = "");
    bool SaveConfig(const std::string& config_path = "");
    const FTBConfig& GetConfig() const { return config_; }
    FTBConfig& GetConfigMutable() { return config_; }

    ftxui::Color GetColor(const std::string& color_name) const;
    ftxui::Color GetFileTypeColor(const std::string& file_type) const;
    ftxui::Color ParseColor(const std::string& color_str) const;
    void ApplyTheme(const std::string& theme_name);
    bool ReloadConfig();
    std::string GetConfigPath() const { return config_path_; }
    bool IsConfigValid() const { return config_loaded_; }
    FTBConfig GetDefaultConfig() const;
    void ResetToDefault();

    // 书签操作
    bool AddBookmark(const std::string& name, const std::string& path);
    bool RemoveBookmark(const std::string& name);
    std::vector<std::pair<std::string, std::string>> GetBookmarks() const;

    // SSH 记录操作
    std::vector<SSHRecord> GetSSHRecords() const;
    void AddSSHRecord(const SSHRecord& record);
    void RemoveSSHRecord(int index);
    void SetSSHRecords(const std::vector<SSHRecord>& records);

public:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

private:
    bool ParseConfigFile(const std::string& content);
    void ApplyColorConfig();
    bool CreateDefaultConfig();
    std::string GetUserHomeDir() const;
    bool ValidateConfig() const;
    void InitializePredefinedColors();

    static std::unique_ptr<ConfigManager> instance_;
    FTBConfig config_;
    std::string config_path_;
    bool config_loaded_;
    std::map<std::string, ftxui::Color> predefined_colors_;
    std::vector<SSHRecord> ssh_records_;
};

// ---- 获取面板边框装饰器（读取 config.ui.panel_border） ----
// 注意：此函数返回无颜色边框，仅供不使用主题颜色的场景使用
// 所有弹窗请使用 detail_element.hpp 中带主题颜色的 GetPanelBorder()

} // namespace FTB

#endif // CONFIG_MANAGER_HPP
