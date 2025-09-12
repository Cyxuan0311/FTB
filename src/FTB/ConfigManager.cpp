#include "FTB/ConfigManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

namespace FTB {

ConfigManager* ConfigManager::instance_ = nullptr;

ConfigManager::ConfigManager() : config_loaded_(false) {
    InitializePredefinedColors();
    config_path_ = GetUserHomeDir() + "/.ftb";
}

ConfigManager* ConfigManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new ConfigManager();
    }
    return instance_;
}

bool ConfigManager::LoadConfig(const std::string& config_path) {
    if (!config_path.empty()) {
        config_path_ = config_path;
    }
    
    // 如果配置文件不存在，创建默认配置
    if (!std::filesystem::exists(config_path_)) {
        if (!CreateDefaultConfig()) {
            std::cerr << "警告: 无法创建默认配置文件" << std::endl;
            config_ = GetDefaultConfig();
            config_loaded_ = true;
            return false;
        }
    }
    
    // 读取配置文件
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        std::cerr << "警告: 无法打开配置文件: " << config_path_ << std::endl;
        config_ = GetDefaultConfig();
        config_loaded_ = true;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    if (!ParseConfigFile(buffer.str())) {
        std::cerr << "警告: 配置文件解析失败，使用默认配置" << std::endl;
        config_ = GetDefaultConfig();
        config_loaded_ = true;
        return false;
    }
    
    config_loaded_ = true;
    ApplyColorConfig();
    std::cout << "配置文件加载成功: " << config_path_ << std::endl;
    return true;
}

bool ConfigManager::SaveConfig(const std::string& config_path) {
    std::string save_path = config_path.empty() ? config_path_ : config_path;
    
    std::ofstream file(save_path);
    if (!file.is_open()) {
        std::cerr << "错误: 无法创建配置文件: " << save_path << std::endl;
        return false;
    }
    
    // 写入颜色配置
    file << "# FTB 配置文件\n\n";
    
    file << "[colors.main]\n";
    file << "background = " << config_.colors_main.background << "\n";
    file << "foreground = " << config_.colors_main.foreground << "\n";
    file << "border = " << config_.colors_main.border << "\n";
    file << "selection_bg = " << config_.colors_main.selection_bg << "\n";
    file << "selection_fg = " << config_.colors_main.selection_fg << "\n\n";
    
    file << "[colors.files]\n";
    file << "directory = " << config_.colors_files.directory << "\n";
    file << "file = " << config_.colors_files.file << "\n";
    file << "executable = " << config_.colors_files.executable << "\n";
    file << "link = " << config_.colors_files.link << "\n";
    file << "hidden = " << config_.colors_files.hidden << "\n";
    file << "system = " << config_.colors_files.system << "\n\n";
    
    file << "[style]\n";
    file << "show_icons = " << (config_.style.show_icons ? "true" : "false") << "\n";
    file << "show_file_size = " << (config_.style.show_file_size ? "true" : "false") << "\n";
    file << "enable_mouse = " << (config_.style.enable_mouse ? "true" : "false") << "\n";
    file << "enable_animations = " << (config_.style.enable_animations ? "true" : "false") << "\n\n";
    
    file << "[layout]\n";
    file << "items_per_page = " << config_.layout.items_per_page << "\n";
    file << "items_per_row = " << config_.layout.items_per_row << "\n";
    file << "detail_panel_ratio = " << config_.layout.detail_panel_ratio << "\n";
    file << "show_detail_panel = " << (config_.layout.show_detail_panel ? "true" : "false") << "\n\n";
    
    file << "[refresh]\n";
    file << "ui_refresh_interval = " << config_.refresh.ui_refresh_interval << "\n";
    file << "content_refresh_interval = " << config_.refresh.content_refresh_interval << "\n";
    file << "auto_refresh = " << (config_.refresh.auto_refresh ? "true" : "false") << "\n\n";
    
    file << "[theme]\n";
    file << "name = " << config_.theme.name << "\n";
    file << "use_colors = " << (config_.theme.use_colors ? "true" : "false") << "\n";
    file << "use_bold = " << (config_.theme.use_bold ? "true" : "false") << "\n";
    file << "use_underline = " << (config_.theme.use_underline ? "true" : "false") << "\n\n";
    
    file << "[mysql]\n";
    file << "default_host = " << config_.mysql.default_host << "\n";
    file << "default_port = " << config_.mysql.default_port << "\n";
    file << "default_username = " << config_.mysql.default_username << "\n";
    file << "default_database = " << config_.mysql.default_database << "\n";
    file << "connection_timeout = " << config_.mysql.connection_timeout << "\n\n";
    
    file << "[ssh]\n";
    file << "default_port = " << config_.ssh.default_port << "\n";
    file << "connection_timeout = " << config_.ssh.connection_timeout << "\n";
    file << "save_connection_history = " << (config_.ssh.save_connection_history ? "true" : "false") << "\n";
    file << "max_connection_history = " << config_.ssh.max_connection_history << "\n\n";
    
    file << "[logging]\n";
    file << "level = " << config_.logging.level << "\n";
    file << "output_to_file = " << (config_.logging.output_to_file ? "true" : "false") << "\n";
    file << "log_file = " << config_.logging.log_file << "\n";
    file << "show_timestamp = " << (config_.logging.show_timestamp ? "true" : "false") << "\n";
    
    file.close();
    std::cout << "配置文件保存成功: " << save_path << std::endl;
    return true;
}

bool ConfigManager::ParseConfigFile(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    std::string current_section;
    
    while (std::getline(stream, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 检查是否是节标题
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // 解析键值对
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 根据节和键设置配置值
            if (current_section == "colors.main") {
                if (key == "background") config_.colors_main.background = value;
                else if (key == "foreground") config_.colors_main.foreground = value;
                else if (key == "border") config_.colors_main.border = value;
                else if (key == "selection_bg") config_.colors_main.selection_bg = value;
                else if (key == "selection_fg") config_.colors_main.selection_fg = value;
            }
            else if (current_section == "colors.files") {
                if (key == "directory") config_.colors_files.directory = value;
                else if (key == "file") config_.colors_files.file = value;
                else if (key == "executable") config_.colors_files.executable = value;
                else if (key == "link") config_.colors_files.link = value;
                else if (key == "hidden") config_.colors_files.hidden = value;
                else if (key == "system") config_.colors_files.system = value;
            }
            else if (current_section == "style") {
                if (key == "show_icons") config_.style.show_icons = (value == "true");
                else if (key == "show_file_size") config_.style.show_file_size = (value == "true");
                else if (key == "enable_mouse") config_.style.enable_mouse = (value == "true");
                else if (key == "enable_animations") config_.style.enable_animations = (value == "true");
            }
            else if (current_section == "layout") {
                if (key == "items_per_page") config_.layout.items_per_page = std::stoi(value);
                else if (key == "items_per_row") config_.layout.items_per_row = std::stoi(value);
                else if (key == "detail_panel_ratio") config_.layout.detail_panel_ratio = std::stod(value);
                else if (key == "show_detail_panel") config_.layout.show_detail_panel = (value == "true");
            }
            else if (current_section == "refresh") {
                if (key == "ui_refresh_interval") config_.refresh.ui_refresh_interval = std::stoi(value);
                else if (key == "content_refresh_interval") config_.refresh.content_refresh_interval = std::stoi(value);
                else if (key == "auto_refresh") config_.refresh.auto_refresh = (value == "true");
            }
            else if (current_section == "theme") {
                if (key == "name") config_.theme.name = value;
                else if (key == "use_colors") config_.theme.use_colors = (value == "true");
                else if (key == "use_bold") config_.theme.use_bold = (value == "true");
                else if (key == "use_underline") config_.theme.use_underline = (value == "true");
            }
            else if (current_section == "mysql") {
                if (key == "default_host") config_.mysql.default_host = value;
                else if (key == "default_port") config_.mysql.default_port = std::stoi(value);
                else if (key == "default_username") config_.mysql.default_username = value;
                else if (key == "default_database") config_.mysql.default_database = value;
                else if (key == "connection_timeout") config_.mysql.connection_timeout = std::stoi(value);
            }
            else if (current_section == "ssh") {
                if (key == "default_port") config_.ssh.default_port = std::stoi(value);
                else if (key == "connection_timeout") config_.ssh.connection_timeout = std::stoi(value);
                else if (key == "save_connection_history") config_.ssh.save_connection_history = (value == "true");
                else if (key == "max_connection_history") config_.ssh.max_connection_history = std::stoi(value);
            }
            else if (current_section == "logging") {
                if (key == "level") config_.logging.level = value;
                else if (key == "output_to_file") config_.logging.output_to_file = (value == "true");
                else if (key == "log_file") config_.logging.log_file = value;
                else if (key == "show_timestamp") config_.logging.show_timestamp = (value == "true");
            }
        }
    }
    
    return true;
}

ftxui::Color ConfigManager::GetColor(const std::string& color_name) const {
    // 首先检查预定义颜色
    auto it = predefined_colors_.find(color_name);
    if (it != predefined_colors_.end()) {
        return it->second;
    }
    
    // 检查自定义颜色
    auto custom_it = config_.custom_colors.find(color_name);
    if (custom_it != config_.custom_colors.end()) {
        return custom_it->second;
    }
    
    // 返回默认颜色
    return ftxui::Color::White;
}

ftxui::Color ConfigManager::GetFileTypeColor(const std::string& file_type) const {
    if (file_type == "directory") return GetColor(config_.colors_files.directory);
    if (file_type == "executable") return GetColor(config_.colors_files.executable);
    if (file_type == "link") return GetColor(config_.colors_files.link);
    if (file_type == "hidden") return GetColor(config_.colors_files.hidden);
    if (file_type == "system") return GetColor(config_.colors_files.system);
    
    return GetColor(config_.colors_files.file);
}

void ConfigManager::ApplyTheme(const std::string& theme_name) {
    if (theme_name == "dark") {
        config_.colors_main.background = "black";
        config_.colors_main.foreground = "white";
        config_.colors_main.border = "blue";
    }
    else if (theme_name == "light") {
        config_.colors_main.background = "white";
        config_.colors_main.foreground = "black";
        config_.colors_main.border = "blue";
    }
    else if (theme_name == "colorful") {
        config_.colors_main.background = "black";
        config_.colors_main.foreground = "white";
        config_.colors_main.border = "magenta";
        config_.colors_files.directory = "cyan";
        config_.colors_files.executable = "green";
        config_.colors_files.link = "yellow";
    }
    else if (theme_name == "minimal") {
        config_.colors_main.background = "black";
        config_.colors_main.foreground = "white";
        config_.colors_main.border = "white";
        config_.style.show_icons = false;
        config_.style.enable_animations = false;
    }
    
    config_.theme.name = theme_name;
    ApplyColorConfig();
}

bool ConfigManager::ReloadConfig() {
    return LoadConfig();
}

FTBConfig ConfigManager::GetDefaultConfig() const {
    return FTBConfig{};
}

void ConfigManager::ResetToDefault() {
    config_ = GetDefaultConfig();
    ApplyColorConfig();
}

bool ConfigManager::CreateDefaultConfig() {
    // 复制模板文件到用户主目录
    std::string template_path = "config/.ftb.template";
    if (!std::filesystem::exists(template_path)) {
        std::cerr << "警告: 模板文件不存在: " << template_path << std::endl;
        return false;
    }
    
    try {
        std::filesystem::copy_file(template_path, config_path_);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: 无法复制模板文件: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::GetUserHomeDir() const {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home);
    }
    return std::string();
}

bool ConfigManager::ValidateConfig() const {
    // 验证颜色值
    auto validateColor = [](const std::string& color) {
        std::vector<std::string> valid_colors = {
            "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
        };
        return std::find(valid_colors.begin(), valid_colors.end(), color) != valid_colors.end();
    };
    
    if (!validateColor(config_.colors_main.background) ||
        !validateColor(config_.colors_main.foreground) ||
        !validateColor(config_.colors_main.border)) {
        return false;
    }
    
    // 验证数值范围
    if (config_.layout.items_per_page <= 0 || config_.layout.items_per_row <= 0 ||
        config_.layout.detail_panel_ratio < 0.0 || config_.layout.detail_panel_ratio > 1.0) {
        return false;
    }
    
    if (config_.refresh.ui_refresh_interval <= 0 || config_.refresh.content_refresh_interval <= 0) {
        return false;
    }
    
    return true;
}

void ConfigManager::ApplyColorConfig() {
    // 应用颜色配置到自定义颜色映射
    config_.custom_colors["main_bg"] = GetColor(config_.colors_main.background);
    config_.custom_colors["main_fg"] = GetColor(config_.colors_main.foreground);
    config_.custom_colors["main_border"] = GetColor(config_.colors_main.border);
    config_.custom_colors["selection_bg"] = GetColor(config_.colors_main.selection_bg);
    config_.custom_colors["selection_fg"] = GetColor(config_.colors_main.selection_fg);
}

void ConfigManager::InitializePredefinedColors() {
    predefined_colors_["black"] = ftxui::Color::Black;
    predefined_colors_["red"] = ftxui::Color::Red;
    predefined_colors_["green"] = ftxui::Color::Green;
    predefined_colors_["yellow"] = ftxui::Color::Yellow;
    predefined_colors_["blue"] = ftxui::Color::Blue;
    predefined_colors_["magenta"] = ftxui::Color::Magenta;
    predefined_colors_["cyan"] = ftxui::Color::Cyan;
    predefined_colors_["white"] = ftxui::Color::White;
    
    // 添加一些自定义颜色
    predefined_colors_["bright_black"] = ftxui::Color::GrayDark;
    predefined_colors_["bright_red"] = ftxui::Color::RedLight;
    predefined_colors_["bright_green"] = ftxui::Color::GreenLight;
    predefined_colors_["bright_yellow"] = ftxui::Color::YellowLight;
    predefined_colors_["bright_blue"] = ftxui::Color::BlueLight;
    predefined_colors_["bright_magenta"] = ftxui::Color::MagentaLight;
    predefined_colors_["bright_cyan"] = ftxui::Color::CyanLight;
    predefined_colors_["bright_white"] = ftxui::Color::GrayLight;
}

} // namespace FTB
