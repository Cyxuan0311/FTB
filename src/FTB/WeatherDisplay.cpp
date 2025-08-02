#include "../include/FTB/WeatherDisplay.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>  
#include <thread>

WeatherDisplay::WeatherDisplay(){
    startWeatherScript(); // 启动天气更新脚本
}

WeatherInfo WeatherDisplay::loadWeatherData() {
    try {
        std::ifstream file("/mnt/f/My__StudyStack/My_Project/FTB_PART/data/weather.json");
        if (!file.is_open()) {
            return {"未知", "N/A", "未知", "N/A", "N/A", "未更新"};
        }

        nlohmann::json j;
        file >> j;

        WeatherInfo info;
        info.city = j["city"];
        info.temperature = j["temperature"];
        info.weather = j["weather"];
        info.high = j["high"];
        info.low = j["low"];
        info.update_time = j["update_time"];
        return info;
    } catch (const std::exception& e) {
        std::cerr << "读取天气数据失败: " << e.what() << std::endl;
        return {"未知", "N/A", "未知", "N/A", "N/A", "未更新"};
    }
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
    auto info = loadWeatherData();
    
    using namespace ftxui;
    
    int temp = std::stoi(info.temperature);
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
                //text("🫤 "),
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

void WeatherDisplay::startWeatherScript() {
    static bool started = false;
    if (started) return;
    started = true;

    std::string weather_file = "/mnt/f/My__StudyStack/My_Project/FTB_PART/data/weather.json";
    if (!std::filesystem::exists(weather_file)) {
        std::ofstream(weather_file).close();
    }

    std::thread([]() {
        std::cout << "正在获取最新天气信息..." << std::endl;
        int result = std::system("python3 /mnt/f/My__StudyStack/My_Project/FTB_PART/fetch_weather.py &");
        if (result != 0) {
            std::cerr << "天气脚本启动失败!" << std::endl;
        }
    }).detach();
}