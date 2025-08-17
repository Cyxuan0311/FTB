#include "../../include/FTB/WeatherService.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>

namespace {
    // 全局单例实例
    std::shared_ptr<WeatherService> g_instance = nullptr;
    std::mutex g_instance_mutex;
}

std::shared_ptr<WeatherService> WeatherService::GetInstance() {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    if (!g_instance) {
        g_instance = std::shared_ptr<WeatherService>(new WeatherService());
    }
    return g_instance;
}

WeatherService::~WeatherService() {
    Stop();
}

bool WeatherService::Start(const WeatherServiceConfig& config) {
    std::lock_guard<std::mutex> config_lock(config_mutex_);
    
    if (running_.load()) {
        LogMessage("WeatherService已经在运行中");
        return true;
    }
    
    // 更新配置
    config_ = config;
    
    return StartInternal();
}

bool WeatherService::StartFromConfig() {
    std::lock_guard<std::mutex> config_lock(config_mutex_);
    
    if (running_.load()) {
        LogMessage("WeatherService已经在运行中");
        return true;
    }
    
    // 从配置文件读取配置
    auto config_manager = ConfigManager::GetInstance();
    if (!config_manager->LoadConfig()) {
        LogError("无法加载配置文件");
        return false;
    }
    
    // 从配置文件构建WeatherServiceConfig
    config_.python_script_path = config_manager->GetString("weather_service.python_script.path");
    config_.weather_data_path = config_manager->GetString("weather_service.data_file.path");
    config_.update_interval = std::chrono::minutes(config_manager->GetInt("weather_service.update.interval_minutes", 30));
    config_.auto_start = config_manager->GetBool("weather_service.update.auto_start", true);
    config_.enable_logging = config_manager->GetBool("weather_service.logging.enabled", true);
    config_.max_retry_attempts = config_manager->GetInt("weather_service.update.max_retry_attempts", 3);
    config_.retry_delay = std::chrono::seconds(config_manager->GetInt("weather_service.update.retry_delay_seconds", 60));
    
    return StartInternal();
}

bool WeatherService::StartInternal() {
    
    // 验证Python脚本路径
    if (!std::filesystem::exists(config_.python_script_path)) {
        std::string error = "Python脚本不存在: " + config_.python_script_path;
        LogError(error);
        NotifyError(error);
        return false;
    }
    
    // 确保数据目录存在
    std::filesystem::path data_dir = std::filesystem::path(config_.weather_data_path).parent_path();
    if (!std::filesystem::exists(data_dir)) {
        try {
            std::filesystem::create_directories(data_dir);
            LogMessage("创建数据目录: " + data_dir.string());
        } catch (const std::exception& e) {
            std::string error = "无法创建数据目录: " + std::string(e.what());
            LogError(error);
            NotifyError(error);
            return false;
        }
    }
    
    // 设置运行标志
    running_.store(true);
    status_.store(WeatherServiceStatus::RUNNING);
    
    // 启动更新线程
    update_thread_ = std::thread(&WeatherService::UpdateLoop, this);
    
    LogMessage("WeatherService启动成功");
    LogMessage("更新间隔: " + std::to_string(config_.update_interval.count()) + " 分钟");
    
    return true;
}

void WeatherService::Stop() {
    if (!running_.load()) {
        return;
    }
    
    LogMessage("正在停止WeatherService...");
    
    // 设置停止标志
    running_.store(false);
    status_.store(WeatherServiceStatus::STOPPED);
    
    // 通知更新线程退出
    update_cv_.notify_all();
    
    // 等待线程结束
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    
    LogMessage("WeatherService已停止");
}

bool WeatherService::UpdateWeather() {
    if (!running_.load()) {
        LogError("WeatherService未运行，无法更新天气数据");
        return false;
    }
    
    status_.store(WeatherServiceStatus::UPDATING);
    LogMessage("开始更新天气数据...");
    
    bool success = false;
    int attempts = 0;
    
    while (attempts < config_.max_retry_attempts && !success) {
        attempts++;
        
        if (attempts > 1) {
            LogMessage("重试第 " + std::to_string(attempts) + " 次...");
            std::this_thread::sleep_for(config_.retry_delay);
        }
        
        success = ExecutePythonScript();
        if (success) {
            success = ReadWeatherData();
        }
    }
    
    if (success) {
        status_.store(WeatherServiceStatus::RUNNING);
        LogMessage("天气数据更新成功");
        NotifyUpdate(current_weather_);
    } else {
        status_.store(WeatherServiceStatus::ERROR);
        std::string error = "天气数据更新失败，已重试 " + std::to_string(attempts) + " 次";
        LogError(error);
        NotifyError(error);
    }
    
    return success;
}

WeatherInfo WeatherService::GetWeatherInfo() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_weather_;
}

WeatherServiceStatus WeatherService::GetStatus() const {
    return status_.load();
}

void WeatherService::SetUpdateCallback(std::function<void(const WeatherInfo&)> callback) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    update_callback_ = std::move(callback);
}

void WeatherService::SetErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    error_callback_ = std::move(callback);
}

bool WeatherService::IsDataValid() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_weather_.is_valid;
}

std::chrono::system_clock::time_point WeatherService::GetLastUpdateTime() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_weather_.last_update;
}

void WeatherService::UpdateLoop() {
    LogMessage("天气更新线程已启动");
    
    while (running_.load()) {
        // 执行天气更新
        UpdateWeather();
        
        // 等待下一次更新
        std::unique_lock<std::mutex> lock(config_mutex_);
        if (update_cv_.wait_for(lock, config_.update_interval, [this] { return !running_.load(); })) {
            break; // 收到停止信号
        }
    }
    
    LogMessage("天气更新线程已退出");
}

bool WeatherService::ExecutePythonScript() {
    try {
        // 构建命令
        std::string command = "python3 \"" + config_.python_script_path + "\"";
        
        LogMessage("执行Python脚本: " + command);
        
        // 执行Python脚本
        int result = std::system(command.c_str());
        
        if (result != 0) {
            LogError("Python脚本执行失败，返回码: " + std::to_string(result));
            return false;
        }
        
        LogMessage("Python脚本执行成功");
        return true;
        
    } catch (const std::exception& e) {
        LogError("执行Python脚本时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool WeatherService::ReadWeatherData() {
    try {
        std::lock_guard<std::mutex> lock(data_mutex_);
        
        // 检查文件是否存在
        if (!std::filesystem::exists(config_.weather_data_path)) {
            LogError("天气数据文件不存在: " + config_.weather_data_path);
            return false;
        }
        
        // 读取JSON文件
        std::ifstream file(config_.weather_data_path);
        if (!file.is_open()) {
            LogError("无法打开天气数据文件: " + config_.weather_data_path);
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        // 解析天气数据
        WeatherInfo new_weather;
        new_weather.city = j.value("city", "未知");
        new_weather.temperature = j.value("temperature", "N/A");
        new_weather.weather = j.value("weather", "未知");
        new_weather.high = j.value("high", "N/A");
        new_weather.low = j.value("low", "N/A");
        new_weather.update_time = j.value("update_time", "未更新");
        new_weather.last_update = std::chrono::system_clock::now();
        
        // 验证数据有效性
        if (ValidateWeatherData(new_weather)) {
            new_weather.is_valid = true;
            current_weather_ = new_weather;
            LogMessage("天气数据读取成功: " + new_weather.city + " " + new_weather.temperature + "°C");
            return true;
        } else {
            LogError("天气数据验证失败");
            return false;
        }
        
    } catch (const nlohmann::json::exception& e) {
        LogError("JSON解析错误: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        LogError("读取天气数据时发生异常: " + std::string(e.what()));
        return false;
    }
}

void WeatherService::LogMessage(const std::string& message) const {
    if (config_.enable_logging) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        std::cout << "[WeatherService] [" << ss.str() << "] " << message << std::endl;
    }
}

void WeatherService::LogError(const std::string& error) const {
    if (config_.enable_logging) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        std::cerr << "[WeatherService] [ERROR] [" << ss.str() << "] " << error << std::endl;
    }
}

bool WeatherService::ValidateWeatherData(const WeatherInfo& info) const {
    // 检查必要字段
    if (info.city.empty() || info.city == "未知") {
        return false;
    }
    
    if (info.temperature.empty() || info.temperature == "N/A") {
        return false;
    }
    
    if (info.weather.empty() || info.weather == "未知") {
        return false;
    }
    
    // 检查温度是否为有效数字
    try {
        if (info.temperature != "N/A") {
            std::stoi(info.temperature);
        }
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

void WeatherService::NotifyUpdate(const WeatherInfo& info) {
    if (update_callback_) {
        try {
            update_callback_(info);
        } catch (const std::exception& e) {
            LogError("更新回调函数执行失败: " + std::string(e.what()));
        }
    }
}

void WeatherService::NotifyError(const std::string& error) {
    if (error_callback_) {
        try {
            error_callback_(error);
        } catch (const std::exception& e) {
            LogError("错误回调函数执行失败: " + std::string(e.what()));
        }
    }
}
