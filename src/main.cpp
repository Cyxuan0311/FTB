#include "../include/file_browser.hpp"
#include "../include/thread_guard.hpp"
#include "../include/file_operations.hpp"
#include "../include/new_file_dialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <stack>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <future>
#include <execution>
#include <iostream>
#include <numeric>
#include "../include/ui_initializer.hpp"
#include "../include/event_handler.hpp"

using namespace ftxui;
namespace fs = std::filesystem;

// 定义文件夹和文件的图标
const std::string FOLDER_ICON = "📁 ";
const std::string FILE_ICON = "📄 ";

// 全局线程相关变量
extern std::mutex cache_mutex;
extern std::unordered_map<std::string, DirectoryCache> dir_cache;

// 动态加载动画字符
const std::vector<std::string> loadingFrames = {
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓",
    "░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ",
    "▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░",
    "▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒",
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓"
};

// calculateSizes lambda（所有数据均通过参数传入或为全局变量）
auto calculateSizes = [](int& selected, std::string& currentPath, std::future<void>& size_future,
                          std::atomic<uintmax_t>& total_folder_size, std::atomic<double>& size_ratio,
                          std::string& selected_size) {
    static std::string last_path;
    static int last_selected = -1;

    if (selected != last_selected || currentPath != last_path) {
        if (size_future.valid() && 
            size_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return;
        }

        size_future = std::async(std::launch::async, [&]() {
            std::lock_guard<std::mutex> lock(cache_mutex);
            auto& cache = dir_cache[currentPath];

            if (!cache.valid) {
                cache.contents = FileBrowser::getDirectoryContents(currentPath);
                cache.last_update = std::chrono::system_clock::now();
                cache.valid = true;
            }

            std::vector<uintmax_t> sizes;
            sizes.reserve(cache.contents.size());
            for (const auto& item : cache.contents) {
                std::string fullPath = (fs::path(currentPath) / item).string();
                sizes.push_back(FileBrowser::getFileSize(fullPath));
            }

            uintmax_t total = std::accumulate(sizes.begin(), sizes.end(), 0ULL);
            total_folder_size.store(total, std::memory_order_relaxed);

            if (selected < static_cast<int>(cache.contents.size())) {
                uintmax_t size = sizes[selected];
                double ratio = total > 0 ? static_cast<double>(size) / total : 0.0;
                size_ratio.store(ratio, std::memory_order_relaxed);

                std::ostringstream stream;
                if (size >= 1024 * 1024) {
                    stream << std::fixed << std::setprecision(2) 
                           << (size / (1024.0 * 1024.0)) << " MB";
                } else if (size >= 1024) {
                    stream << std::fixed << std::setprecision(2) 
                           << (size / 1024.0) << " KB";
                } else {
                    stream << size << " B";
                }
                selected_size = stream.str();
            }
        });
        last_selected = selected;
        last_path = currentPath;
    }
};

int main() {
    int hovered_index = -1;
    std::stack<std::string> pathHistory;
    std::string currentPath = fs::absolute(".").string();
    auto allContents = FileBrowser::getDirectoryContents(currentPath);
    std::vector<std::string> filteredContents = allContents;
    int selected = 0;
    auto screen = ScreenInteractive::Fullscreen();

    std::atomic<double> size_ratio(0.0);
    std::atomic<uintmax_t> total_folder_size(0);
    std::string selected_size;
    std::future<void> size_future;

    std::atomic<bool> refresh_ui{true};
    std::thread timer([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    std::string searchQuery;
    auto ui_pair = initializeUI(searchQuery, filteredContents, selected,
                              pathHistory, currentPath, allContents);
    auto& component = ui_pair.first;
    auto& searchInput = ui_pair.second;

    //渲染器函数
    auto renderer = Renderer(component, [&] {
        calculateSizes(selected, currentPath, size_future, total_folder_size, size_ratio, selected_size);
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);
        std::string time_str = FileBrowser::formatTime(now_tm);

        std::string displayPath = fs::canonical(currentPath).string();
        if (displayPath.length() > 40) {
            displayPath = "..." + displayPath.substr(displayPath.length() - 37);
        }

        if (!searchQuery.empty()) {
            filteredContents.clear();
            for (const auto& item : allContents) {
                if (item.find(searchQuery) != std::string::npos) {
                    filteredContents.push_back(item);
                }
            }
        } else {
            filteredContents = allContents;
        }

        // 构造文件项
        Elements elements;
        for (size_t i = 0; i < filteredContents.size(); ++i) {
            std::string fullPath = currentPath + "/" + filteredContents[i];
            bool is_dir = FileBrowser::isDirectory(fullPath);

            auto text_color = is_dir ? 
                color(Color::RGB(135, 206, 250)) : 
                color(Color::RGB(255, 99, 71));

            Decorator bg_style = nothing;
            if (hovered_index == (int)i) {
                bg_style = bgcolor(Color::RGB(120, 120, 120)) | bold;
            }
            if (selected == (int)i) {
                bg_style = bgcolor(Color::RGB(255, 255, 0)) | color(Color::Black) | bold;
            }

            std::string itemText = filteredContents[i];
            std::string icon = is_dir ? FOLDER_ICON : FILE_ICON;
            size_t pos = itemText.find(searchQuery);
            if (pos != std::string::npos) {
                Elements highlighted;
                if (pos > 0) {
                    highlighted.push_back(text(itemText.substr(0, pos)));
                }
                highlighted.push_back(text(itemText.substr(pos, searchQuery.length())) | color(Color::Yellow));
                if (pos + searchQuery.length() < itemText.length()) {
                    highlighted.push_back(text(itemText.substr(pos + searchQuery.length())));
                }
                elements.push_back(
                    hbox({
                        text(selected == (int)i ? "→ " : "  "),
                        text(icon),
                        hbox(highlighted) | bold | text_color,
                        filler()
                    }) | border | bg_style | size(WIDTH, LESS_THAN, 50)
                );
            } else {
                elements.push_back(
                    hbox({
                        text(selected == (int)i ? "→ " : "  "),
                        text(icon),
                        text(itemText) | bold | text_color,
                        filler()
                    }) | border | bg_style | size(WIDTH, LESS_THAN, 50)
                );
            }
        }

        // 多列布局
        const int max_items_per_column = 5;
        std::vector<Elements> columns;
        for (size_t start = 0; start < elements.size(); start += max_items_per_column) {
            auto end = std::min(start + max_items_per_column, elements.size());
            columns.emplace_back(elements.begin() + start, elements.begin() + end);
        }

        Elements column_boxes;
        for (auto& column : columns) {
            column_boxes.push_back(vbox(std::move(column)) | flex);
        }

        static int loadingIndex = 0;
        std::string loadingIndicator = loadingFrames[loadingIndex % loadingFrames.size()];
        loadingIndex = (loadingIndex + 1) % (loadingFrames.size() * 2);  // 控制动画速度
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio.load() * 100);

        // 注意：使用 Elements{ ... } 显式构造初始化列表
        return vbox(Elements{
            hbox({
                text("🤖当前路径: " + displayPath) | bold | color(Color::Pink1) | flex,
                filler(),
                vbox(Elements{
                    hbox({
                        text(" █ ") | color(Color::Cyan),
                        text(selected_size)
                    }) | size(WIDTH, LESS_THAN, 25),
                    hbox({
                        text("["),
                        gauge(size_ratio.load()) | flex | color(Color::Green) | size(WIDTH, EQUAL, 20),
                        text("]")
                    }),
                    hbox({
                        text(" ▓ ") | color(Color::Yellow),
                        text(ratio_stream.str() + "%") | bold
                    })
                }) | border | size(WIDTH, LESS_THAN, 30),

                vbox(Elements{
                    text("快捷键说明：") | color(Color::Orange4) | bold,
                    text("↑/↓ 导航文件列表") | size(HEIGHT,LESS_THAN,1),
                    text("Enter 进入目录") | size(HEIGHT,LESS_THAN,1),
                    text("Backspace 返回上级") | size(HEIGHT,LESS_THAN,1),
                    text("F键 新建文件夹/K键 新建文件") | size(HEIGHT,LESS_THAN,1),
                    text("D键 删除文件夹或文件") | size(HEIGHT,LESS_THAN,1),
                    text("ESC 退出程序") | color(Color::Red3) | size(HEIGHT,LESS_THAN,1)
                }) | border | size(WIDTH, LESS_THAN, 30),

                vbox(Elements{
                    text(time_str) | color(Color::GrayDark),
                    text(loadingIndicator) | color(Color::Green)
                }) | border
            }) | size(HEIGHT, EQUAL, 9),
            
            searchInput->Render() | border | color(Color::Magenta),
            hbox(column_boxes) | frame | border | color(Color::Blue) | flex
            
        });
    });

    //最终的组件集成
    auto final_component = CatchEvent(renderer, [&](ftxui::Event event) {
        return handleEvents(event, pathHistory, currentPath, allContents, filteredContents, selected, searchQuery, screen,refresh_ui);
    });

    screen.Loop(final_component);
    return 0;
}
