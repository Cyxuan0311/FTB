#include "../include/file_browser.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <stack>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>  // 新增用于控制小数位
#include <sstream>  // 新增字符串流处理

using namespace ftxui;

// 进入目录的逻辑封装成函数
void enterDirectory(std::stack<std::string>& pathHistory, std::string& currentPath, std::vector<std::string>& contents, int& selected) {
    if (!contents.empty() && selected < contents.size()) {
        std::string fullPath = currentPath + "/" + contents[selected];
        if (FileBrowser::isDirectory(fullPath)) {
            pathHistory.push(currentPath);
            currentPath = fullPath;
            contents = FileBrowser::getDirectoryContents(currentPath);
            selected = 0;
        }
    }
}

// RAII 线程管理类
class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t) : thread_(t) {}
    ~ThreadGuard() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
private:
    std::thread& thread_;
};

// 动态加载动画字符
const std::string loadingChars = "|/-\\";

int main() {
    int hovered_index = -1;  // 新增悬停索引
    std::stack<std::string> pathHistory;
    std::string currentPath = ".";
    auto allContents = FileBrowser::getDirectoryContents(currentPath);
    std::vector<std::string> filteredContents = allContents;
    int selected = 0;
    auto&& screen = ScreenInteractive::Fullscreen();
    double size_ratio = 0.0;
    
    // 添加缺失的声明 ↓
    std::string selected_size; 
    uintmax_t total_folder_size = 0;
    // 自动刷新定时器
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

    // 菜单配置
    MenuOption menu_option;
    menu_option.on_enter = [&] {
        enterDirectory(pathHistory, currentPath, filteredContents, selected);
        allContents = FileBrowser::getDirectoryContents(currentPath);
        filteredContents = allContents;
        searchQuery.clear();
    };

    auto selector = Menu(&filteredContents, &selected, menu_option);

    // 添加鼠标支持组件
    auto mouse_component = CatchEvent(selector, [&](Event event) {
        // 处理鼠标移动事件
        if (event.is_mouse() && event.mouse().motion == Mouse::Moved) {
            hovered_index = selector->OnEvent(event) ? selected : -1;
            return true;
        }
        // 处理鼠标点击事件
        if (event.is_mouse() && event.mouse().button == Mouse::Left && 
            event.mouse().motion == Mouse::Pressed) {
            menu_option.on_enter();
            return true;
        }
        return false;
    });

    auto component = Container::Vertical({
        searchInput,
        mouse_component
    });

    // ... 保持其他代码不变 ...

auto calculateSizes = [&] {
    static int last_selected = -1;
    static std::string last_path;
    
    if (selected != last_selected || currentPath != last_path) {
        if (!filteredContents.empty() && selected < filteredContents.size()) {
            std::string selected_path = currentPath + "/" + filteredContents[selected];
            uintmax_t selected_size_bytes = FileBrowser::getFileSize(selected_path);
            
            // 单位转换
            std::ostringstream size_stream;
            if (selected_size_bytes >= 1024*1024) {
                size_stream << std::fixed << std::setprecision(2) 
                          << (selected_size_bytes/(1024.0*1024.0)) << " MB";
            } else {
                size_stream << std::fixed << std::setprecision(2) 
                          << (selected_size_bytes/1024.0) << " KB";
            }
            selected_size = size_stream.str();
            
            // 新增：计算当前目录总大小 ↓↓↓
            total_folder_size = 0;
            for (const auto& item : allContents) {
                std::string full_path = currentPath + "/" + item;
                total_folder_size += FileBrowser::getFileSize(full_path);
            }
            // 新增结束 ↑↑↑

            size_ratio = total_folder_size > 0 ? 
                static_cast<double>(selected_size_bytes) / total_folder_size : 0.0;
                
            last_selected = selected;
            last_path = currentPath;
        }
    }
};

    int loadingIndex = 0;
    auto renderer = Renderer(component, [&] {
        calculateSizes();
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);
        std::string time_str = FileBrowser::formatTime(now_tm);

        // 过滤内容
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

        Elements elements;
        for (size_t i = 0; i < filteredContents.size(); ++i) {
            std::string fullPath = currentPath + "/" + filteredContents[i];
            bool is_dir = FileBrowser::isDirectory(fullPath);

            // 颜色配置
            auto text_color = is_dir ? 
                color(Color::RGB(65, 105, 225)) :  // 文件夹蓝色
                color(Color::RGB(220, 20, 60));    // 文件红色

            // 背景样式（悬停 + 选中）
            ftxui::Decorator bg_style = nothing;
            if (hovered_index == (int)i) {
                // 鼠标悬停时增加亮度提示
                bg_style = bgcolor(Color::RGB(120, 120, 120));  // 悬停更亮的灰色背景
            }
            if (selected == (int)i) {
                bg_style = bgcolor(Color::RGB(255, 255, 0)) | color(Color::Black);
            }

            // 高亮搜索结果
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

        // 动态加载动画
        std::string loadingIndicator(1, loadingChars[loadingIndex % loadingChars.length()]);
        loadingIndex++;
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio * 100);

        return vbox({
            hbox({
                // 当前路径和加载指示器
                text("当前路径: " + currentPath) | bold | color(Color::White) | flex,
                filler(),
                
                // 右侧信息面板（调整后的紧凑布局）
                vbox({
                    hbox({
                        text(" █ ") | color(Color::Cyan),
                        text(selected_size) 
                    }) | size(WIDTH, LESS_THAN, 25),
                    hbox({
                        text("["),
                        gauge(size_ratio) | flex | color(Color::Green) | size(WIDTH, EQUAL, 20),
                        text("]")
                    }),
                    hbox({
                        text(" ▓ ") | color(Color::Yellow),
                        text(ratio_stream.str() + "%") 
                    }) 
                }) | border | size(WIDTH, LESS_THAN, 30),
                
                // 时间显示
                vbox({
                    text(time_str) | color(Color::GrayDark),
                    text(loadingIndicator) | color(Color::Green)
                }) | border
            }) | size(HEIGHT, EQUAL, 5),
            
            // 搜索框和文件列表
            searchInput->Render() | border | color(Color::Magenta),
            vbox(elements) | frame | border | color(Color::Blue) | flex
        });
    });

    // 事件处理
    auto final_component = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Escape) {
            refresh_ui = false;
            screen.Exit();
            return true;
        }
        if (event == Event::Backspace || event == Event::ArrowLeft) {
            if (!pathHistory.empty()) {
                currentPath = pathHistory.top();
                pathHistory.pop();
                allContents = FileBrowser::getDirectoryContents(currentPath);
                filteredContents = allContents;
                searchQuery.clear();
                selected = 0;
                return true;
            }
        }
        return false;
    });

    screen.Loop(final_component);
    return 0;
}