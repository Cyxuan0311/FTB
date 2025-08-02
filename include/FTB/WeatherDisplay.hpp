#pragma once

#include <string>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>
#include <cstdlib>  
#include <thread>

struct WeatherInfo {
    std::string city;
    std::string temperature;
    std::string weather;
    std::string high;
    std::string low;
    std::string update_time;
};

class WeatherDisplay {
public:
    static ftxui::Element render();
    static void startWeatherScript();
    WeatherDisplay();

private:
    static WeatherInfo loadWeatherData();
    static std::string getWeatherEmoji(const std::string& weather);
    static ftxui::Color getTemperatureColor(int temp);
};