#include "../include/UIManager.hpp"
#include "../include/FileManager.hpp"
#include "../include/ThreadGuard.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <stack>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <filesystem>
#include <algorithm>
#include <cmath>

using namespace ftxui;
namespace fs = std::filesystem;

// 文件与文件夹图标
const std::string FOLDER_ICON = "📁 ";
const std::string FILE_ICON = "📄 ";

// 加载动画帧
const std::vector<std::string> loadingFrames = {
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓", 
    "░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ", 
    "▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░",
    "▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒",
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓"
};

int main() {
    int hovered_index = -1;
    std::stack<std::string> pathHistory;
    std::string currentPath = fs::absolute(".").string();
    auto allContents = FileManager::getDirectoryContents(currentPath);
    std::vector<std::string> filteredContents = allContents;
    int selected = 0;
    auto screen = ScreenInteractive::Fullscreen();

    std::atomic<double> size_ratio(0.0);
    std::atomic<uintmax_t> total_folder_size(0);
    std::string selected_size;
    std::future<void> size_future;

    std::atomic<double> wave_progress(0.0);
    std::atomic<bool> refresh_ui{true};

    // 启动波浪动画的线程
    std::thread wave_thread([&] {
        while (refresh_ui) {
            wave_progress.store(wave_progress.load() + 0.1, std::memory_order_relaxed);
            if (wave_progress.load() > 2 * M_PI) { 
                wave_progress.store(0.0, std::memory_order_relaxed);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 控制动画速度
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard waveGuard(wave_thread);

    // 启动定时器线程
    std::thread timer([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    std::string searchQuery;
    auto ui_pair = UIManager::initializeUI(searchQuery, filteredContents, selected, pathHistory, currentPath, allContents);
    auto& component = ui_pair.first;
    auto& searchInput = ui_pair.second;

    // 异步计算文件大小及比例的 Lambda
    auto calculateSizes = [&](){
        static std::string last_path;
        static int last_selected = -1;
        if (selected != last_selected || currentPath != last_path) {
            if (size_future.valid() && size_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                return;
            }
            int local_selected = selected;
            std::string local_currentPath = currentPath;
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                auto& cache = FileManager::dir_cache[local_currentPath];
                if (!cache.valid) {
                    cache.contents = FileManager::getDirectoryContents(local_currentPath);
                    cache.last_update = std::chrono::system_clock::now();
                    cache.valid = true;
                }
                if (cache.contents.empty()) {
                    total_folder_size.store(0, std::memory_order_relaxed);
                    size_ratio.store(0.0, std::memory_order_relaxed);
                    selected_size = "0 B";
                    last_selected = selected;
                    last_path = currentPath;
                    return;
                }
            }
            size_future = std::async(std::launch::async, [local_selected, local_currentPath, &total_folder_size, &size_ratio, &selected_size]() {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                auto& cache = FileManager::dir_cache[local_currentPath];
                if (!cache.valid) {
                    cache.contents = FileManager::getDirectoryContents(local_currentPath);
                    cache.last_update = std::chrono::system_clock::now();
                    cache.valid = true;
                }
                std::vector<uintmax_t> sizes;
                sizes.reserve(cache.contents.size());
                for (const auto& item : cache.contents) {
                    std::string fullPath = (fs::path(local_currentPath) / item).string();
                    sizes.push_back(FileManager::getFileSize(fullPath));
                }
                uintmax_t total = std::accumulate(sizes.begin(), sizes.end(), 0ULL);
                total_folder_size.store(total, std::memory_order_relaxed);
                if (!cache.contents.empty() && local_selected < static_cast<int>(cache.contents.size())) {
                    uintmax_t size = sizes[local_selected];
                    double ratio = total > 0 ? static_cast<double>(size) / total : 0.0;
                    size_ratio.store(ratio, std::memory_order_relaxed);
                    std::ostringstream stream;
                    if (size >= 1024 * 1024) {
                        stream << std::fixed << std::setprecision(2) << (size / (1024.0 * 1024.0)) << " MB";
                    } else if (size >= 1024) {
                        stream << std::fixed << std::setprecision(2) << (size / 1024.0) << " KB";
                    } else {
                        stream << size << " B";
                    }
                    selected_size = stream.str();
                } else {
                    selected_size = "0 B";
                    std::cerr << "Directory empty or selection out of range" << std::endl;
                }
            });
            last_selected = selected;
            last_path = currentPath;
        }
    };

    // 波浪动画的 Lambda
    auto waveGauge = [&] {
        Elements wave_elements;
        for (int i = 0; i < 10; ++i) { // 控制波浪的高度
            double progress = wave_progress.load() + i * 0.4; // 让每行的波浪错开
            double wave_value = (std::sin(progress) + 1) / 2; // 转换成 0 ~ 1 之间的值
    
            // 这里将 gauge 放入 vbox 形成纵向波浪
            wave_elements.push_back(gauge(wave_value) | color(Color::BlueLight) | size(HEIGHT, LESS_THAN, 1));
        }
        return vbox(wave_elements) | borderDouble | color(Color::RGB(33,136,143)) | size(HEIGHT, EQUAL, 10) | size(WIDTH,EQUAL,40);
    };

    // 渲染 UI 的 Lambda
    auto renderer = Renderer(component, [&] {
        calculateSizes();
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);
        std::string time_str = FileManager::formatTime(now_tm);

        std::string displayPath = fs::canonical(currentPath).string();
        if (displayPath.length() > 40) {
            displayPath = "..." + displayPath.substr(displayPath.length() - 37);
        }
        if (!searchQuery.empty()) {
            filteredContents.clear();
            for (const auto & item : allContents) {
                if (item.find(searchQuery) != std::string::npos) {
                    filteredContents.push_back(item);
                }
            }
        } else {
            filteredContents = allContents;
        }
        Elements elements;
        for (size_t i = 0; i < filteredContents.size(); ++i) {
            std::string fullPath = currentPath + "/" + filteredContents[i];
            bool is_dir = FileManager::isDirectory(fullPath);
            auto text_color = is_dir ? color(Color::RGB(135, 206, 250)) : color(Color::RGB(255, 99, 71));
            Decorator bg_style = nothing;
            if (hovered_index == (int)i) {
                bg_style = bgcolor(Color::RGB(120, 120, 120)) | bold;
            }
            if (selected == (int)i) {
                bg_style = bgcolor(Color::GrayLight) | color(Color::Black) | bold;
            }
            std::string itemText = filteredContents[i];
            std::string icon = is_dir ? FOLDER_ICON : FILE_ICON;
            size_t pos = itemText.find(searchQuery);
            if (pos != std::string::npos && !searchQuery.empty()) {
                Elements highlighted;
                if (pos > 0) {
                    highlighted.push_back(text(itemText.substr(0, pos)));
                }
                highlighted.push_back(text(itemText.substr(pos, searchQuery.length())) | color(Color::GrayLight));
                if (pos + searchQuery.length() < itemText.length()) {
                    highlighted.push_back(text(itemText.substr(pos + searchQuery.length())));
                }
                elements.push_back(hbox({
                    text(selected == (int)i ? "→ " : "  "),
                    text(icon),
                    hbox(highlighted) | bold | text_color | (selected == (int)i ? underlined : nothing),
                    filler()
                }) | borderHeavy | bg_style | size(WIDTH, LESS_THAN, 50));
            } else {
                elements.push_back(hbox({
                    text(selected == (int)i ? "→ " : "  "),
                    text(icon),
                    text(itemText) | bold | text_color | (selected == (int)i ? underlined : nothing),
                    filler()
                }) | borderHeavy | bg_style | size(WIDTH, LESS_THAN, 50));
            }
        }
        const int max_items_per_column = 5;
        std::vector<Elements> columns;
        for (size_t start = 0; start < elements.size(); start += max_items_per_column) {
            auto end = std::min(start + max_items_per_column, elements.size());
            columns.emplace_back(elements.begin() + start, elements.begin() + end);
        }
        Elements column_boxes;
        for (auto & column : columns) {
            column_boxes.push_back(vbox(std::move(column)) | flex);
        }
        static int loadingIndex = 0;
        std::string loadingIndicator = loadingFrames[loadingIndex % loadingFrames.size()];
        loadingIndex = (loadingIndex + 1) % (loadingFrames.size() * 2);
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio.load() * 100);
        return vbox({
            hbox({
                vbox({  // 让它们成为一个整体
                    hbox({  // ✅ 让 FTB 和当前路径框水平排列
                        text("FTB") | bold | borderDouble | bgcolor(Color::BlueLight) | size(WIDTH, LESS_THAN, 5),
                        
                        filler() | size(WIDTH, EQUAL, 2),  // FTB 和路径框之间的间距
                        
                        text("🤖当前路径: " + displayPath) | bold | borderHeavy | color(Color::Pink1) | size(HEIGHT, LESS_THAN, 1) | flex  
                    }),
                    
                    waveGauge() | size(HEIGHT, EQUAL, 10) | size(WIDTH, LESS_THAN, 75)  // ✅ 波浪框放在下方
                }) | size(WIDTH, EQUAL, 80)  ,// 限制整体宽度，确保美观
                
                
                filler(),
                
                vbox({
                    hbox({text(" █ ") | color(Color::Cyan), text(selected_size)}) | size(WIDTH, LESS_THAN, 25),
                    hbox({text("[") | color(Color::Yellow3), gauge(size_ratio.load()) | flex | color(Color::Green) | size(WIDTH, EQUAL, 20), text("]") | color(Color::Yellow3)}),
                    hbox({text(" ▓ ") | color(Color::Yellow), text(ratio_stream.str() + "%") | bold})
                }) | border | color(Color::Purple3) | size(HEIGHT, EQUAL, 3),
        
                vbox({
                    text("快捷键说明：") | color(Color::Orange4) | bold,
                    text("↑/↓ 导航文件列表") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("Enter 进入目录") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("Backspace 返回上级") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("Ctrl+k键 新建文件夹") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("Ctrl+f键 新建文件") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("空格键查看选中文件夹细节") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("Delete键 删除文件夹或文件") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
                    text("ESC 退出程序") | color(Color::Red3) | size(HEIGHT, LESS_THAN, 1)
                }) | border | color(Color::Purple3) | size(WIDTH, LESS_THAN, 35) | size(HEIGHT, EQUAL, 15),
        
                vbox({
                    text(time_str) | color(Color::GrayDark),
                    text(loadingIndicator) | color(Color::Green)
                }) | borderDouble | color(Color::Purple3) | size(HEIGHT, EQUAL, 5)    
            }),
        
            searchInput->Render() | border | color(Color::Magenta) | size(WIDTH, LESS_THAN, 100),
        
            hbox(column_boxes) | color(Color::Blue) | frame | borderDashed | color(Color::GrayDark) | flex,
        
            gauge(1) | color(Color::RGB(158,160,161)) | size(WIDTH, EQUAL, 190)
        });        
    });

    auto final_component = CatchEvent(renderer, [&](Event event) {
        return UIManager::handleEvents(event, pathHistory, currentPath, allContents, filteredContents, selected, searchQuery, screen, refresh_ui);
    });

    screen.Loop(final_component);
    return 0;
}
