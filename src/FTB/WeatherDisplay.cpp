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
            std::cout << "WeatherDisplay初始化成功" << std::endl;
            initialized_ = true;
        } else {
            std::cerr << "WeatherDisplay初始化失败" << std::endl;
        }
        
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
        return "☺️";
    if (weather.find("多云") != std::string::npos)
        return "😑";
    if (weather.find("阴") != std::string::npos)
        return "😰";
    if (weather.find("雨") != std::string::npos)
        return "🫤";
    if (weather.find("雪") != std::string::npos)
        return "🥶";
    return "❓";
}

ftxui::Color WeatherDisplay::getTemperatureColor(int temp) {
    if (temp >= 30) return ftxui::Color::Red;
    if (temp >= 20) return ftxui::Color::Orange1;
    if (temp >= 10) return ftxui::Color::Yellow;
    return ftxui::Color::Blue;
}

ftxui::Element WeatherDisplay::render() {
    // 确保天气服务已初始化
    if (!initialized_ || !weather_service_) {
        initialize();
    }
    
    // 获取天气数据
    auto info = weather_service_->GetWeatherInfo();
    
    using namespace ftxui;
    
    // 检查数据有效性
    if (!weather_service_->IsDataValid()) {
        return vbox({
            text("🌍 天气状况") | bold | color(Color::Orange4),
            text("数据加载中...") | color(Color::GrayLight) | center,
            text("🕒 请稍候") | color(Color::GrayDark) | center
        }) | frame | borderHeavy | color(Color::RGB(35,124,148)) | size(WIDTH, LESS_THAN, 30);
    }
    
    int temp = 0;
    try {
        temp = std::stoi(info.temperature);
    } catch (const std::exception&) {
        temp = 20; // 默认温度
    }
    
    auto tempColor = getTemperatureColor(temp);
    
    return vbox({
        // 标题栏
        hbox({
            text("🌍 ") | bold,
            text("天气状况") | bold | color(Color::Orange4),
            text("   "),
            text(info.city) | color(Color::GrayLight)
        }),
        // 天气信息部分
        vbox({
            hbox({
                text(getWeatherEmoji(info.weather)) | color(Color::Cyan),
                text(" " + info.weather) | color(Color::Cyan)
            }) | center,
            hbox({
                text(info.temperature) | bold | color(tempColor)
            }) | center,
            hbox({
                text("↓ " + info.low) | color(Color::Blue),
                text(" ~ ") | color(Color::GrayLight),
                text(info.high + " ↑") | color(Color::Red)
            }) | center
        }) | flex,
        separator(),
        text("🕒 " + info.update_time) | color(Color::GrayDark)
    }) | frame | borderHeavy | color(Color::RGB(35,124,148)) | size(WIDTH, LESS_THAN, 30);
}

void WeatherDisplay::onWeatherUpdate(const WeatherInfo& info) {
    std::cout << "天气数据已更新: " << info.city << " " << info.temperature << "°C" << std::endl;
}

void WeatherDisplay::onWeatherError(const std::string& error) {
    std::cerr << "天气数据更新错误: " << error << std::endl;
}