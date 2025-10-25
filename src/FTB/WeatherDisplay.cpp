#include "../include/FTB/WeatherDisplay.hpp"
#include "../include/FTB/WeatherService.hpp"
#include <iostream>

// 静态成员初始化
std::shared_ptr<WeatherService> WeatherDisplay::weather_service_ = nullptr;
bool WeatherDisplay::initialized_ = false;

WeatherDisplay::WeatherDisplay() {
    initialize();
}

void WeatherDisplay::initialize() {
    if (initialized_) {
        return;
    }
    
    try {
        // 获取WeatherService实例
        weather_service_ = WeatherService::GetInstance();
        
        // 设置回调函数
        weather_service_->SetUpdateCallback(onWeatherUpdate);
        weather_service_->SetErrorCallback(onWeatherError);
        
        // 从配置文件启动天气服务
        if (weather_service_->StartFromConfig()) {
            // 静默成功日志
        } else {
            std::cerr << "WeatherDisplay初始化失败，但将继续运行（天气功能可能不可用）" << std::endl;
        }
        
        // 无论天气服务是否启动成功，都标记为已初始化
        // 这样界面仍然可以显示，只是天气功能不可用
        initialized_ = true;
        
    } catch (const std::exception& e) {
        std::cerr << "WeatherDisplay初始化异常: " << e.what() << std::endl;
    }
}

void WeatherDisplay::cleanup() {
    if (weather_service_) {
        weather_service_->Stop();
        weather_service_ = nullptr;
    }
    initialized_ = false;
}

std::string WeatherDisplay::getWeatherEmoji(const std::string& weather) {
    if (weather.find("晴") != std::string::npos)
        return "☀";
    if (weather.find("多云") != std::string::npos)
        return "☁";
    if (weather.find("阴") != std::string::npos)
        return "☁";
    if (weather.find("雨") != std::string::npos)
        return "☂";
    if (weather.find("雪") != std::string::npos)
        return "❄";
    return "?";
}

ftxui::Color WeatherDisplay::getTemperatureColor(int temp) {
    if (temp >= 30) return ftxui::Color::Red;
    if (temp >= 20) return ftxui::Color::Orange1;
    if (temp >= 10) return ftxui::Color::Yellow;
    return ftxui::Color::Blue;
}

ftxui::Element WeatherDisplay::render() {
    using namespace ftxui;
    
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // 尝试获取真实天气数据，如果失败则使用模拟数据
    std::string weather_icon = "☁";
    std::string weather_text = "多云";
    std::string city = "武汉";
    int temp = 28;
    int low_temp = 24;
    int high_temp = 32;
    ftxui::Color weather_color = Color::Cyan;
    
    // 如果天气服务可用且数据有效，使用真实数据
    if (initialized_ && weather_service_ && weather_service_->IsDataValid()) {
        try {
            auto info = weather_service_->GetWeatherInfo();
            city = info.city;
            weather_text = info.weather;
            
            // 解析温度
            try {
                temp = std::stoi(info.temperature);
            } catch (...) {
                temp = 28; // 默认温度
            }
            
            // 解析高低温度
            try {
                low_temp = std::stoi(info.low);
                high_temp = std::stoi(info.high);
            } catch (...) {
                low_temp = temp - 4;
                high_temp = temp + 4;
            }
            
            // 根据天气设置图标和颜色
            if (weather_text.find("晴") != std::string::npos) {
                weather_icon = "☀";
                weather_color = Color::Yellow;
            } else if (weather_text.find("雨") != std::string::npos) {
                weather_icon = "☂";
                weather_color = Color::Blue;
            } else if (weather_text.find("云") != std::string::npos) {
                weather_icon = "☁";
                weather_color = Color::Cyan;
            } else if (weather_text.find("雪") != std::string::npos) {
                weather_icon = "❄";
                weather_color = Color::White;
            }
        } catch (...) {
            // 如果获取真实数据失败，使用模拟数据
        }
    }
    
    // 如果没有真实数据，使用基于时间的模拟数据
    if (!initialized_ || !weather_service_ || !weather_service_->IsDataValid()) {
        int hour = tm.tm_hour;
        if (hour >= 6 && hour < 12) {
            weather_icon = "☀";
            weather_text = "晴朗";
            weather_color = Color::Yellow;
        } else if (hour >= 12 && hour < 18) {
            weather_icon = "☀";
            weather_text = "晴朗";
            weather_color = Color::Yellow;
        } else if (hour >= 18 && hour < 22) {
            weather_icon = "☀";
            weather_text = "傍晚";
            weather_color = Color::Orange1;
        } else {
            weather_icon = "☽";
            weather_text = "夜晚";
            weather_color = Color::Blue;
        }
        
        // 模拟温度（基于时间）
        temp = 20 + (hour - 12) * 2;
        if (temp < 15) temp = 15;
        if (temp > 35) temp = 35;
        low_temp = temp - 4;
        high_temp = temp + 4;
    }
    
    Elements weather_elements;
    
    // 标题
    weather_elements.push_back(
        hbox({
            text(weather_icon) | bold | color(weather_color),
            text(" 天气") | bold | color(Color::White)
        }) | center
    );
    
    weather_elements.push_back(separator());
    
    // 天气信息
    Elements info_elements;
    info_elements.push_back(
        hbox({
            text(weather_text) | color(weather_color) | bold
        }) | center
    );
    
    info_elements.push_back(
        hbox({
            text(std::to_string(temp) + "°C") | bold | color(Color::Orange1) | size(WIDTH, EQUAL, 8)
        }) | center
    );
    
    info_elements.push_back(
        hbox({
            text("↓ " + std::to_string(low_temp) + "°") | color(Color::Blue),
            text(" ~ ") | color(Color::GrayLight),
            text(std::to_string(high_temp) + "° ↑") | color(Color::Red)
        }) | center
    );
    
    weather_elements.push_back(vbox(info_elements) | flex);
    weather_elements.push_back(separator());
    
    // 底部信息
    weather_elements.push_back(
        hbox({
            text(city) | color(Color::GrayLight),
            text("  "),
            text("🕒 " + std::to_string(tm.tm_hour) + ":" + 
                 (tm.tm_min < 10 ? "0" : "") + std::to_string(tm.tm_min)) | color(Color::GrayDark)
        }) | center
    );
    
    return vbox(weather_elements) | frame | borderRounded | color(Color::RGB(30, 144, 255)) | 
           bgcolor(Color::RGB(25, 25, 35)) | size(WIDTH, LESS_THAN, 25) | size(HEIGHT, EQUAL, 8);
}

void WeatherDisplay::onWeatherUpdate(const WeatherInfo& info) {
    // 减少日志输出，避免干扰主界面
    // std::cout << "天气数据已更新: " << info.city << " " << info.temperature << "°C" << std::endl;
    
    // 触发界面刷新
    if (weather_service_) {
        // 这里可以添加界面刷新逻辑
        // 由于FTXUI的渲染是自动的，数据更新后界面会自动重新渲染
        (void)info; // 避免未使用参数警告
    }
}

void WeatherDisplay::onWeatherError(const std::string& error) {
    std::cerr << "天气数据更新错误: " << error << std::endl;
}