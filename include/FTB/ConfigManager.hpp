#pragma once

#include <string>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

/**
 * @brief 配置管理类
 * 
 * 提供以下功能：
 * - 读取YAML配置文件
 * - 提供类型安全的配置访问
 * - 支持配置验证和默认值
 * - 支持配置热重载
 */
class ConfigManager {
public:
    /**
     * @brief 获取ConfigManager单例实例
     * @return ConfigManager实例的智能指针
     */
    static std::shared_ptr<ConfigManager> GetInstance();
    
    /**
     * @brief 析构函数
     */
    ~ConfigManager() = default;
    
    /**
     * @brief 加载配置文件
     * @param config_path 配置文件路径
     * @return 是否成功加载
     */
    bool LoadConfig(const std::string& config_path = "config/weather_config.yaml");
    
    /**
     * @brief 重新加载配置文件
     * @return 是否成功重新加载
     */
    bool ReloadConfig();
    
    /**
     * @brief 获取配置根节点
     * @return YAML节点
     */
    const YAML::Node& GetRoot() const;
    
    /**
     * @brief 获取字符串配置值
     * @param key 配置键（支持点号分隔的路径）
     * @param default_value 默认值
     * @return 配置值
     */
    std::string GetString(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief 获取整数配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    int GetInt(const std::string& key, int default_value = 0) const;
    
    /**
     * @brief 获取布尔配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    bool GetBool(const std::string& key, bool default_value = false) const;
    
    /**
     * @brief 获取浮点数配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    double GetDouble(const std::string& key, double default_value = 0.0) const;
    
    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 是否存在
     */
    bool HasKey(const std::string& key) const;
    
    /**
     * @brief 获取配置文件的最后修改时间
     * @return 最后修改时间
     */
    std::time_t GetLastModifiedTime() const;
    
    /**
     * @brief 验证配置文件的完整性
     * @return 是否有效
     */
    bool ValidateConfig() const;
    
    /**
     * @brief 获取配置错误信息
     * @return 错误信息
     */
    std::string GetLastError() const;
    
    /**
     * @brief 设置配置监听器（用于热重载）
     * @param callback 回调函数
     */
    void SetConfigChangeCallback(std::function<void()> callback);
    
    /**
     * @brief 获取当前配置文件路径
     * @return 配置文件路径
     */
    std::string GetConfigPath() const;

private:
    // 私有构造函数，实现单例模式
    ConfigManager() = default;
    
    // 禁用拷贝构造和赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // 成员变量
    YAML::Node config_root_;
    std::string config_path_;
    std::time_t last_modified_time_;
    std::string last_error_;
    std::function<void()> config_change_callback_;
    
    // 私有方法
    YAML::Node GetNodeByPath(const std::string& path) const;
    std::vector<std::string> SplitPath(const std::string& path) const;
    bool IsConfigFileModified() const;
    void LogError(const std::string& error) const;
};
