#pragma once

#include <string>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>
#include <memory>

// 前向声明
class WeatherService;
struct WeatherInfo;

class WeatherDisplay {
public:
    static ftxui::Element render();
    static void initialize();
    static void cleanup();
    WeatherDisplay();

private:
    static std::string getWeatherEmoji(const std::string& weather);
    static ftxui::Color getTemperatureColor(int temp);
    static void onWeatherUpdate(const WeatherInfo& info);
    static void onWeatherError(const std::string& error);
    
    static std::shared_ptr<WeatherService> weather_service_;
    static bool initialized_;
};