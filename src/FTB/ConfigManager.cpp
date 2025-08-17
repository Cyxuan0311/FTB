#include "../../include/FTB/ConfigManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace {
    // 全局单例实例
    std::shared_ptr<ConfigManager> g_instance = nullptr;
    std::mutex g_instance_mutex;
}

std::shared_ptr<ConfigManager> ConfigManager::GetInstance() {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (!g_instance) {
        g_instance = std::shared_ptr<ConfigManager>(new ConfigManager());
    }
    return g_instance;
}

bool ConfigManager::LoadConfig(const std::string& config_path) {
    try {
        config_path_ = config_path;
        
        // 检查配置文件是否存在
        if (!std::filesystem::exists(config_path_)) {
            last_error_ = "配置文件不存在: " + config_path_;
            LogError(last_error_);
            return false;
        }
        
        // 读取YAML文件
        config_root_ = YAML::LoadFile(config_path_);
        
        // 更新最后修改时间
        last_modified_time_ = std::filesystem::last_write_time(config_path_).time_since_epoch().count();
        
        // 验证配置
        if (!ValidateConfig()) {
            return false;
        }
        
        std::cout << "配置文件加载成功: " << config_path_ << std::endl;
        return true;
        
    } catch (const YAML::Exception& e) {
        last_error_ = "YAML解析错误: " + std::string(e.what());
        LogError(last_error_);
        return false;
    } catch (const std::exception& e) {
        last_error_ = "配置文件加载异常: " + std::string(e.what());
        LogError(last_error_);
        return false;
    }
}

bool ConfigManager::ReloadConfig() {
    return LoadConfig(config_path_);
}

const YAML::Node& ConfigManager::GetRoot() const {
    return config_root_;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& default_value) const {
    try {
        YAML::Node node = GetNodeByPath(key);
        if (node && node.IsScalar()) {
            return node.as<std::string>();
        }
        return default_value;
    } catch (const std::exception& e) {
        LogError("获取字符串配置失败: " + key + " - " + e.what());
        return default_value;
    }
}

int ConfigManager::GetInt(const std::string& key, int default_value) const {
    try {
        YAML::Node node = GetNodeByPath(key);
        if (node && node.IsScalar()) {
            return node.as<int>();
        }
        return default_value;
    } catch (const std::exception& e) {
        LogError("获取整数配置失败: " + key + " - " + e.what());
        return default_value;
    }
}

bool ConfigManager::GetBool(const std::string& key, bool default_value) const {
    try {
        YAML::Node node = GetNodeByPath(key);
        if (node && node.IsScalar()) {
            return node.as<bool>();
        }
        return default_value;
    } catch (const std::exception& e) {
        LogError("获取布尔配置失败: " + key + " - " + e.what());
        return default_value;
    }
}

double ConfigManager::GetDouble(const std::string& key, double default_value) const {
    try {
        YAML::Node node = GetNodeByPath(key);
        if (node && node.IsScalar()) {
            return node.as<double>();
        }
        return default_value;
    } catch (const std::exception& e) {
        LogError("获取浮点数配置失败: " + key + " - " + e.what());
        return default_value;
    }
}

bool ConfigManager::HasKey(const std::string& key) const {
    try {
        YAML::Node node = GetNodeByPath(key);
        return node && !node.IsNull();
    } catch (const std::exception&) {
        return false;
    }
}

std::time_t ConfigManager::GetLastModifiedTime() const {
    return last_modified_time_;
}

bool ConfigManager::ValidateConfig() const {
    try {
        // 检查必要的配置项
        std::vector<std::string> required_keys = {
            "weather_service.python_script.path",
            "weather_service.data_file.path",
            "weather_service.update.interval_minutes"
        };
        
        for (const auto& key : required_keys) {
            if (!HasKey(key)) {
                last_error_ = "缺少必要的配置项: " + key;
                LogError(last_error_);
                return false;
            }
        }
        
        // 验证Python脚本路径
        std::string script_path = GetString("weather_service.python_script.path");
        if (!std::filesystem::exists(script_path)) {
            last_error_ = "Python脚本不存在: " + script_path;
            LogError(last_error_);
            return false;
        }
        
        // 验证数据文件目录
        std::string data_path = GetString("weather_service.data_file.path");
        std::filesystem::path data_dir = std::filesystem::path(data_path).parent_path();
        if (!std::filesystem::exists(data_dir)) {
            // 尝试创建目录
            try {
                std::filesystem::create_directories(data_dir);
            } catch (const std::exception& e) {
                last_error_ = "无法创建数据目录: " + data_dir.string() + " - " + e.what();
                LogError(last_error_);
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "配置验证异常: " + std::string(e.what());
        LogError(last_error_);
        return false;
    }
}

std::string ConfigManager::GetLastError() const {
    return last_error_;
}

void ConfigManager::SetConfigChangeCallback(std::function<void()> callback) {
    config_change_callback_ = std::move(callback);
}

std::string ConfigManager::GetConfigPath() const {
    return config_path_;
}

YAML::Node ConfigManager::GetNodeByPath(const std::string& path) const {
    std::vector<std::string> path_parts = SplitPath(path);
    YAML::Node current = config_root_;
    
    for (const auto& part : path_parts) {
        if (current && current[part]) {
            current = current[part];
        } else {
            return YAML::Node();
        }
    }
    
    return current;
}

std::vector<std::string> ConfigManager::SplitPath(const std::string& path) const {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

bool ConfigManager::IsConfigFileModified() const {
    if (config_path_.empty()) {
        return false;
    }
    
    try {
        auto current_time = std::filesystem::last_write_time(config_path_).time_since_epoch().count();
        return current_time > last_modified_time_;
    } catch (const std::exception&) {
        return false;
    }
}

void ConfigManager::LogError(const std::string& error) const {
    std::cerr << "[ConfigManager] ERROR: " << error << std::endl;
}
