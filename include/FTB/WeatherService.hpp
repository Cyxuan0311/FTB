#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <functional>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include "ConfigManager.hpp"

/**
 * @brief 天气信息数据结构
 */
struct WeatherInfo {
    std::string city;
    std::string temperature;
    std::string weather;
    std::string high;
    std::string low;
    std::string update_time;
    std::chrono::system_clock::time_point last_update;
    bool is_valid = false;
};

/**
 * @brief 天气服务配置结构
 */
struct WeatherServiceConfig {
    std::string python_script_path = "/mnt/f/My__StudyStack/My_Project/FTB/data/Get_weather_information.py";
    std::string weather_data_path = "/mnt/f/My__StudyStack/My_Project/FTB/data/weather.json";
    std::chrono::minutes update_interval = std::chrono::minutes(30);
    bool auto_start = true;
    bool enable_logging = true;
    int max_retry_attempts = 3;
    std::chrono::seconds retry_delay = std::chrono::seconds(60);
};

/**
 * @brief 天气服务状态枚举
 */
enum class WeatherServiceStatus {
    STOPPED,
    RUNNING,
    UPDATING,
    ERROR
};

/**
 * @brief 专业的天气服务类
 * 
 * 提供以下功能：
 * - 单例模式确保全局唯一实例
 * - 线程安全的天气数据访问
 * - 自动定期更新天气数据
 * - 错误处理和重试机制
 * - 可配置的更新策略
 * - 优雅的启动和停止
 */
class WeatherService {
public:
    /**
     * @brief 获取WeatherService单例实例
     * @return WeatherService实例的智能指针
     */
    static std::shared_ptr<WeatherService> GetInstance();
    
    /**
     * @brief 析构函数
     */
    ~WeatherService();
    
    /**
     * @brief 启动天气服务
     * @param config 服务配置（可选，如果不提供则从配置文件读取）
     * @return 是否成功启动
     */
    bool Start(const WeatherServiceConfig& config = WeatherServiceConfig{});
    
    /**
     * @brief 从配置文件启动天气服务
     * @return 是否成功启动
     */
    bool StartFromConfig();
    
    /**
     * @brief 停止天气服务
     */
    void Stop();
    
    /**
     * @brief 手动触发天气更新
     * @return 是否成功更新
     */
    bool UpdateWeather();
    
    /**
     * @brief 获取当前天气信息
     * @return 天气信息结构
     */
    WeatherInfo GetWeatherInfo() const;
    
    /**
     * @brief 获取服务状态
     * @return 当前服务状态
     */
    WeatherServiceStatus GetStatus() const;
    
    /**
     * @brief 设置配置更新回调函数
     * @param callback 回调函数
     */
    void SetUpdateCallback(std::function<void(const WeatherInfo&)> callback);
    
    /**
     * @brief 设置错误回调函数
     * @param callback 错误回调函数
     */
    void SetErrorCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief 检查天气数据是否有效
     * @return 数据是否有效
     */
    bool IsDataValid() const;
    
    /**
     * @brief 获取上次更新时间
     * @return 上次更新时间
     */
    std::chrono::system_clock::time_point GetLastUpdateTime() const;

private:
    // 私有构造函数，实现单例模式
    WeatherService() = default;
    
    // 禁用拷贝构造和赋值
    WeatherService(const WeatherService&) = delete;
    WeatherService& operator=(const WeatherService&) = delete;
    
    // 成员变量
    mutable std::mutex data_mutex_;
    mutable std::mutex config_mutex_;
    std::atomic<WeatherServiceStatus> status_{WeatherServiceStatus::STOPPED};
    std::atomic<bool> running_{false};
    
    WeatherInfo current_weather_;
    WeatherServiceConfig config_;
    
    std::thread update_thread_;
    std::condition_variable update_cv_;
    
    std::function<void(const WeatherInfo&)> update_callback_;
    std::function<void(const std::string&)> error_callback_;
    
    // 私有方法
    bool StartInternal();
    void UpdateLoop();
    bool ExecutePythonScript();
    bool ReadWeatherData();
    void LogMessage(const std::string& message) const;
    void LogError(const std::string& error) const;
    bool ValidateWeatherData(const WeatherInfo& info) const;
    void NotifyUpdate(const WeatherInfo& info);
    void NotifyError(const std::string& error);
};
