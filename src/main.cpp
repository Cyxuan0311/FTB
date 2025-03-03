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
#include <future>       // 添加future支持
#include <execution>    // 添加并行算法支持
#include <iostream>     // 添加cerr支持
#include <numeric>      // 添加reduce支持

using namespace ftxui;
namespace fs = std::filesystem;

// 线程相关的全局变量声明
extern std::mutex cache_mutex;
extern std::unordered_map<std::string, DirectoryCache> dir_cache;

// 动态加载动画字符
const std::vector<std::string> loadingFrames = {
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓",  // 第一阶段
    "░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ",  // 第二阶段 
    "▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░",  // 第三阶段
    "▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒",  // 第四阶段
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓"   // 重复循环
};

int main() {
    int hovered_index = -1;
    std::stack<std::string> pathHistory;
    std::string currentPath = fs::absolute(".").string();
    auto allContents = FileBrowser::getDirectoryContents(currentPath);
    std::vector<std::string> filteredContents = allContents;
    int selected = 0;
    auto&& screen = ScreenInteractive::Fullscreen();

    std::atomic<double> size_ratio(0.0);
    std::atomic<uintmax_t> total_folder_size(0);
    std::string selected_size;

    std::atomic<bool> refresh_ui{true};
    std::thread timer([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    std::string searchQuery;
    auto searchInput = Input(&searchQuery, "搜索...");

    MenuOption menu_option;
    menu_option.on_enter = [&] {
        enterDirectory(pathHistory, currentPath, filteredContents, selected);
        allContents = FileBrowser::getDirectoryContents(currentPath);
        filteredContents = allContents;
        searchQuery.clear();
    };

    auto selector = Menu(&filteredContents, &selected, menu_option);

    auto mouse_component = CatchEvent(selector, [&](Event event) {
        if (event.is_mouse() && event.mouse().motion == Mouse::Moved) {
            hovered_index = selector->OnEvent(event) ? selected : -1;
            return true;
        }
        if (event.is_mouse() && event.mouse().button == Mouse::Left && 
            event.mouse().motion == Mouse::Pressed) {
            menu_option.on_enter();
            return true;
        }
        return false;
    });

    auto component = Container::Vertical({searchInput, mouse_component});

    std::future<void> size_future;
    auto calculateSizes = [&] {
        static std::string last_path;
        static int last_selected = -1;

        if (selected != last_selected || currentPath != last_path) {
            if (size_future.valid() && 
                size_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                return;
            }

            size_future = std::async(std::launch::async, [&] {
                std::lock_guard<std::mutex> lock(cache_mutex);
                auto& cache = dir_cache[currentPath];

                if (!cache.valid) {
                    cache.contents = FileBrowser::getDirectoryContents(currentPath);
                    cache.last_update = std::chrono::system_clock::now();
                }

                std::vector<uintmax_t> sizes;
                sizes.reserve(cache.contents.size());
                std::for_each(std::execution::par_unseq, cache.contents.begin(), cache.contents.end(),
                    [&](const auto& item) {
                        sizes.push_back(FileBrowser::getFileSize((fs::path(currentPath) / item).string()));
                    });

                uintmax_t total = std::reduce(sizes.begin(), sizes.end());
                total_folder_size.store(total, std::memory_order_relaxed);

                if (selected < cache.contents.size()) {
                    uintmax_t size = sizes[selected];
                    double ratio = total > 0 ? static_cast<double>(size) / total : 0.0;
                    size_ratio.store(ratio, std::memory_order_relaxed);

                    std::ostringstream stream;
                    if (size >= 1024*1024) {
                        stream << std::fixed << std::setprecision(2) 
                             << (size/(1024.0*1024.0)) << " MB";
                    } else {
                        stream << std::fixed << std::setprecision(2) 
                             << (size/1024.0) << " KB";
                    }
                    selected_size = stream.str();
                }
            });
            last_selected = selected;
            last_path = currentPath;
        }
    };

    int loadingIndex = 0;
    auto renderer = Renderer(component, [&] {
        calculateSizes();
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

        // 文件项创建
        Elements elements;
        for (size_t i = 0; i < filteredContents.size(); ++i) {
            std::string fullPath = currentPath + "/" + filteredContents[i];
            bool is_dir = FileBrowser::isDirectory(fullPath);

            auto text_color = is_dir ? 
                color(Color::RGB(135, 206, 250)) : 
                color(Color::RGB(255, 99, 71));

            ftxui::Decorator bg_style = nothing;
            if (hovered_index == (int)i) {
                bg_style = bgcolor(Color::RGB(120, 120, 120)) | bold;
            }
            if (selected == (int)i) {
                bg_style = bgcolor(Color::RGB(255, 255, 0)) | color(Color::Black) | bold;
            }

            std::string itemText = filteredContents[i];
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
                        hbox(highlighted) | bold | text_color,
                        filler()
                    }) | border | bg_style | size(WIDTH, LESS_THAN, 50)
                );
            } else {
                elements.push_back(
                    hbox({
                        text(selected == (int)i ? "→ " : "  "),
                        text(itemText) | bold | text_color,
                        filler()
                    }) | border | bg_style | size(WIDTH, LESS_THAN, 50)
                );
            }
        }

        // 多列布局实现
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

        std::string loadingIndicator = loadingFrames[loadingIndex % loadingFrames.size()];
        loadingIndex = (loadingIndex + 1) % (loadingFrames.size() * 2);  // 控制动画速度
        loadingIndex++;
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio.load() * 100);

        return vbox({
            hbox({
                text("当前路径: " + displayPath) | bold | color(Color::White) | flex,
                filler(),
                vbox({
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
                vbox({
                    text(time_str) | color(Color::GrayDark),
                    text(loadingIndicator) | color(Color::Green)
                }) | border
            }) | size(HEIGHT, EQUAL, 5),
            searchInput->Render() | border | color(Color::Magenta),
            hbox(column_boxes) | frame | border | color(Color::Blue) | flex
        });
    });

    auto final_component = CatchEvent(renderer, [&](Event event) {
        try {
            if (event == Event::Escape) {
                refresh_ui = false;
                screen.Exit();
                return true;
            }
            if (event == Event::Backspace || event == Event::ArrowLeft) {
                fs::path current(currentPath);
                if (current.has_parent_path() || !pathHistory.empty()) {
                    pathHistory.push(currentPath);
                    currentPath = current.has_parent_path() ? 
                        current.parent_path().string() : pathHistory.top();
    
                    currentPath = fs::canonical(currentPath).string();
                    dir_cache[currentPath].valid = false;
    
                    std::thread([&] {
                        std::lock_guard<std::mutex> lock(cache_mutex);
                        allContents = FileBrowser::getDirectoryContents(currentPath);
                        filteredContents = allContents;
                    }).detach();
    
                    searchQuery.clear();
                    selected = 0;
                    return true;
                }
            }
            if (event == Event::Character('K')) {
                // 新建文件
                auto newFileName = showNewFileDialog(screen);
                if (!newFileName.empty()) {
                    fs::path fullPath = fs::path(currentPath) / newFileName;
                    try {
                        if (createFile(fullPath.string())) {  // 尝试创建文件
                            std::lock_guard<std::mutex> lock(cache_mutex);
                            allContents = FileBrowser::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            dir_cache[currentPath].valid = false;  // 使缓存无效，以便重新加载
                        } else {
                            std::cerr << "Failed to create file: " << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error creating file: " << e.what() << std::endl;
                    }
                }
                return true;
            }
            if (event == Event::Character('F')) {
                // 新建文件夹
                std::string dirName;
                auto dirNameInput = Input(&dirName, "文件夹名");
                auto cancelButton = Button("取消", [&] {
                    screen.Exit();
                });
                auto createButton = Button("创建", [&] {
                    if (!dirName.empty()) {
                        fs::path fullPath = fs::path(currentPath) / dirName;
                        try {
                            if (createDirectory(fullPath.string())) {  // 尝试创建文件夹
                                std::lock_guard<std::mutex> lock(cache_mutex);
                                allContents = FileBrowser::getDirectoryContents(currentPath);
                                filteredContents = allContents;
                                dir_cache[currentPath].valid = false;  // 使缓存无效，以便重新加载
                            } else {
                                std::cerr << "Failed to create directory: " << fullPath.string() << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error creating directory: " << e.what() << std::endl;
                        }
                    }
                    screen.Exit();
                });
                auto container = Container::Vertical({
                    dirNameInput,
                    Container::Horizontal({
                        cancelButton,
                        createButton
                    })
                });
                auto renderer = Renderer(container, [&] {
                    return vbox({
                        text("新建文件夹"),
                        dirNameInput->Render(),
                        hbox({
                            cancelButton->Render(),
                            filler(),
                            createButton->Render()
                        }) | hcenter  // 按钮水平居中
                    }) | borderRounded | color(Color::Blue3Bis) 
                      | size(WIDTH, GREATER_THAN, 40)  // 最小宽度
                      | vcenter | hcenter;  // 同时应用水平和垂直居中
                });
                screen.Loop(renderer);
                return true;
            }
            if (event == Event::Character('D')) {
                // 删除文件或文件夹
                if (selected >= 0 && selected < filteredContents.size()) {
                    std::string itemName = filteredContents[selected];
                    fs::path fullPath = fs::path(currentPath) / itemName;
                    try {
                        if (deleteFileOrDirectory(fullPath.string())) {
                            std::lock_guard<std::mutex> lock(cache_mutex);
                            allContents = FileBrowser::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            dir_cache[currentPath].valid = false;  // 使缓存无效，以便重新加载
                        } else {
                            std::cerr << "Failed to delete: " << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error deleting: " << e.what() << std::endl;
                    }
                }
                return true;
            }
        } catch (const std::exception& e) {
            refresh_ui = false;
            screen.Exit();
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return false;
    });

    screen.Loop(final_component);
    return 0;
}