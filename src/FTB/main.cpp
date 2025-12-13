#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>

// 引入项目中各个功能模块的头文件
#include "../include/FTB/ClipboardManager.hpp"
#include "../include/FTB/DirectoryHistory.hpp"
#include "../include/FTB/FileManager.hpp"
#include "../include/FTB/FileSizeCalculator.hpp"
#include "../include/FTB/ThreadGuard.hpp"
#include "../include/FTB/HandleManager/UIManager.hpp"
#include "../include/FTB/IconMapper.hpp"
#include "../include/FTB/Vim/Vim_Like.hpp"
#include "../include/FTB/ObjectPool.hpp"
#include "../include/FTB/AsyncFileManager.hpp"
#include "../include/FTB/WeatherDisplay.hpp"
#include "../include/FTB/WeatherService.hpp"
#include "../include/FTB/detail_element.hpp"
#include "../include/FTB/ConfigManager.hpp"
#include "../include/FTB/ThemeManager.hpp"
#include "../include/FTB/showing/MarioAnimation.hpp"

using namespace ftxui;
namespace fs = std::filesystem;

// 马里奥动画实例
FTB::MarioAnimation marioAnimation;

// 加载动画帧，用于显示"波浪"或加载进度效果
const std::vector<std::string> loadingFrames = {
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓",
    "░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ",
    "▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░",
    "▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒",
    " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓"
};

int main()
{
    // ---------- 变量声明与初始化部分 ----------
    
    // 初始化配置管理器
    auto config_manager = FTB::ConfigManager::GetInstance();
    if (!config_manager->LoadConfig()) {
        std::cerr << "警告: 配置文件加载失败，使用默认配置" << std::endl;
    }
    
    // 初始化主题管理器
    FTB::ThemeManager::GetInstance();
    
    // 初始化异步文件管理器
    FTB::GlobalAsyncFileManager::initialize();
    
    // 启用马里奥动画的高帧率模式以获得更流畅的动画
    marioAnimation.setHighFrameRateMode(true);
    
    // 应用主题配置到布局设置
    const auto& config = config_manager->GetConfig();
    // 布局配置变量（将在后面使用）
    int items_per_page = config.layout.items_per_page;
    int items_per_row = config.layout.items_per_row;
    int detail_width = static_cast<int>(config.layout.detail_panel_ratio * 100);
    
    // 初始化天气服务
    auto weather_service = WeatherService::GetInstance();
    if (!weather_service->StartFromConfig()) {
        std::cerr << "警告: 天气服务启动失败，天气功能可能不可用" << std::endl;
    }

    int hovered_index = -1;  // 鼠标悬停时的索引，-1 表示未悬停
    DirectoryHistory directoryHistory;  // 保存目录浏览历史的对象
    std::string currentPath = fs::absolute(".").string();  // 当前工作目录，默认为程序启动目录

    // 获取当前路径下的所有文件和文件夹名称
    auto allContents = FileManager::getDirectoryContents(currentPath);
    // filteredContents 用于存储经过搜索过滤后的结果，初始时等于 allContents
    std::vector<std::string> filteredContents = allContents;
    int selected = 0;  // 当前选中的项目索引，默认为第一个（索引 0）

    // 创建全屏的交互式屏幕对象
    auto screen = ScreenInteractive::Fullscreen();

    // ---------- 分页相关变量 ----------

    int current_page = 0;               // 当前页码，从 0 开始
    // items_per_page 和 items_per_row 已在上面从配置中初始化
    // 计算总页数：向上取整
    int total_pages = (static_cast<int>(filteredContents.size()) + items_per_page - 1) / items_per_page;

    // ---------- 文件大小计算相关 ----------

    std::atomic<double> size_ratio(0.0);        // 当前选中项已计算的大小占总大小的比例（0.0 - 1.0）
    std::atomic<uintmax_t> total_folder_size(0); // 当前选中项的总字节大小
    std::string selected_size;                   // 用于显示在界面上的格式化大小字符串
    std::future<void> size_future;               // 用于异步计算文件夹大小

    // ---------- “波浪”动画进度条相关 ----------

    std::atomic<double> wave_progress(0.0); // 用于控制加载动画的当前进度
    std::atomic<bool> refresh_ui{true};     // 控制 UI 刷新的标志，设为 false 即可停止刷新

    // 启动一个后台线程，不断更新 wave_progress 以驱动马里奥跳跃动画
    std::thread wave_thread([&] {
        while (refresh_ui)
        {
            // 根据12帧动画调整进度，使跳跃动作更自然流畅
            // 放慢动画速度，让用户更好地欣赏动画细节
            wave_progress.store(wave_progress.load() + 0.06, std::memory_order_relaxed);
            if (wave_progress.load() > 2 * M_PI)
                wave_progress.store(0.0, std::memory_order_relaxed);
            // 120ms 刷新一次，更优雅的动画节奏
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            // 通知屏幕线程进行一次重绘
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard waveGuard(wave_thread);  // 使用 RAII 管理线程生命周期，防止程序退出时线程未 join

    // 再启动一个定时器线程，每 150ms 向屏幕线程发送自定义事件，用于触发整体 UI 刷新
    std::thread timer([&] {
        while (refresh_ui)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);  // 管理定时器线程生命周期

    // ---------- 搜索输入框相关 ----------

    std::string searchQuery;  // 当前的搜索关键词
    // 初始化 UI，获得根组件和搜索输入框对应的组件指针
    auto ui_pair = UIManager::initializeUI(searchQuery, filteredContents, selected, directoryHistory, currentPath, allContents);
    auto& component   = ui_pair.first;   // 根组件，用于整体渲染和事件捕获
    auto& searchInput = ui_pair.second;  // 搜索输入框组件

    // 用于异步计算目录或文件大小的函数
    auto calculateSizes = [&]() {
        FileSizeCalculator::CalculateSizes(currentPath, selected, total_folder_size, size_ratio, selected_size);
    };

    // 简化的天气显示 - 静态显示，减少复杂性

    // 构建马里奥运动动画的渲染函数
    auto marioAnimationRenderer = [&] {
        return marioAnimation.render(wave_progress.load());
    };

    // ---------- 主渲染函数（Renderer） ----------

    auto renderer = Renderer(component, [&] {
        // 简化的天气显示 - 直接渲染，不进行复杂的更新检查
        
        // 每次渲染时更新文件大小信息
        calculateSizes();

        // 获取当前系统时间并格式化为字符串
        auto time_now = std::chrono::system_clock::now();
        std::time_t now_c  = std::chrono::system_clock::to_time_t(time_now);
        std::tm     now_tm = *std::localtime(&now_c);
        std::string time_str = FileManager::formatTime(now_tm);

        // 获取当前路径的绝对路径并截断到 40 个字符以内
        std::string displayPath = fs::canonical(currentPath).string();
        if (displayPath.length() > 40)
        {
            displayPath = "..." + displayPath.substr(displayPath.length() - 37);
        }

        // 如果有搜索关键词，则对 allContents 进行过滤，生成 filteredContents，且重置到第一页
        if (!searchQuery.empty())
        {
            filteredContents.clear();
            for (const auto& item : allContents)
            {
                if (item.find(searchQuery) != std::string::npos)
                    filteredContents.push_back(item);
            }
            current_page = 0;  // 搜索时跳到第一页
        }
        else
        {
            // 否则恢复到未过滤的列表
            filteredContents = allContents;
        }

        // 重新计算总页数
        total_pages = (static_cast<int>(filteredContents.size()) + items_per_page - 1) / items_per_page;
        if (total_pages == 0)
            total_pages = 1;
        // 保证当前页号在合法范围内
        if (current_page >= total_pages) current_page = total_pages - 1;
        if (current_page < 0) current_page = 0;

        // 计算当前页的起始和结束索引
        int start_index = current_page * items_per_page;
        int end_index   = std::min(start_index + items_per_page, static_cast<int>(filteredContents.size()));

        // 如果选中的索引不在当前页范围内，则跳转到选中项所在的页
        if (selected < start_index || selected >= end_index)
        {
            current_page = selected / items_per_page;
            start_index  = current_page * items_per_page;
            end_index    = std::min(start_index + items_per_page, static_cast<int>(filteredContents.size()));
        }

        // 构建网格布局：将当前页内的项目切分成若干行
        std::vector<Elements> grid_rows;
        for (int i = start_index; i < end_index; i += items_per_row)
        {
            Elements row_elements;
            for (int j = 0; j < items_per_row && (i + j) < end_index; ++j)
            {
                int         index    = i + j;
                fs::path item_path = fs::path(currentPath) / filteredContents[index];
                bool     is_dir    = FileManager::isDirectory(item_path.string());  // 判断是否文件夹

                // 文件夹和文件分别使用不同颜色
                auto text_color = is_dir
                    ? color(Color::RGB(135, 206, 250))
                    : color(Color::RGB(255, 99, 71));

                // 选中或悬停时使用不同背景或文字效果
                Decorator bg_style = nothing;
                if (hovered_index == index)
                    bg_style = bgcolor(Color::RGB(120, 120, 120)) | bold;
                if (selected == index)
                    bg_style = bgcolor(Color::GrayLight) | color(Color::Black) | bold;

                std::string itemText = filteredContents[index];
                std::string icon     = FTB::Icons::GetIconForPath(item_path, is_dir);
                size_t      pos      = itemText.find(searchQuery);  // 搜索高亮

                // 定义每个项目的固定宽度和高度
                const int item_width  = 28;
                const int item_height = 1;

                // 如果搜索关键词在文件名中，则将匹配部分高亮显示
                if (pos != std::string::npos && !searchQuery.empty())
                {
                    Elements highlighted;
                    if (pos > 0)
                        highlighted.push_back(text(itemText.substr(0, pos)));
                    highlighted.push_back(
                        text(itemText.substr(pos, searchQuery.size())) |
                        color(Color::GrayLight)
                    );
                    if (pos + searchQuery.size() < itemText.size())
                        highlighted.push_back(text(itemText.substr(pos + searchQuery.size())));

                    // 将图标、匹配文本、箭头等组合成一行
                    row_elements.push_back(
                        vbox({
                            hbox({
                                text(selected == index ? "→ " : "  "),
                                text(icon),
                                hbox(highlighted) | bold | text_color |
                                (selected == index ? underlined : nothing)
                            }) | flex,
                            // 固定高度为 0，避免额外留空白
                            text(" ") | size(HEIGHT, EQUAL, 0)
                        }) |
                        borderHeavy |
                        bg_style |
                        size(WIDTH, EQUAL, item_width) |
                        size(HEIGHT, EQUAL, item_height) |
                        yflex_grow
                    );
                }
                else
                {
                    // 普通项目（无搜索匹配）显示
                    row_elements.push_back(
                        vbox({
                            hbox({
                                text(selected == index ? "→ " : "  "),
                                text(icon),
                                text(itemText) | bold | text_color |
                                (selected == index ? underlined : nothing)
                            }) | flex,
                            text(" ") | size(HEIGHT, EQUAL, 0)
                        }) |
                        borderHeavy |
                        bg_style |
                        size(WIDTH, EQUAL, item_width) |
                        size(HEIGHT, EQUAL, item_height) |
                        yflex_grow
                    );
                }
            }
            // 将该行的项目水平排列，并设置高度为 5 行
            grid_rows.push_back(row_elements);
        }

        // 将每一行网格元素（Elements 向量）拼接成整体的网格布局
        Elements grid_elements;
        for (const auto& row : grid_rows)
        {
            grid_elements.push_back(hbox(std::move(row)) | size(HEIGHT, EQUAL, 5));
        }

        // 动态加载指示符：取 loadingFrames 中的当前帧
        static int loadingIndex = 0;
        std::string loadingIndicator = loadingFrames[loadingIndex % loadingFrames.size()];
        loadingIndex = (loadingIndex + 1) % (loadingFrames.size() * 2);

        // 将 size_ratio 转换为 0-100 的百分比字符串
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio.load() * 100);

        // 构建分页信息条：显示当前页码 / 总页数，以及总项目数量
        auto page_info =
            hbox({
                text("Page: ") | color(Color::Yellow),
                text(std::to_string(current_page + 1) + "/" + std::to_string(total_pages)) | bold,
                text(" (") | color(Color::Yellow),
                text(std::to_string(filteredContents.size()) + " items") | color(Color::GrayDark),
                text(")") | color(Color::Yellow)
            }) |
            border |
            color(Color::Green);

        // ---------- 构建最终的界面布局 ----------

        return vbox({
            // 第一行：标题栏 + 加载动画 + 文件大小进度 + 天气显示 + 时间 / 加载指示符
            hbox({
                // 左侧：应用名称和路径信息 + 马里奥动画
                vbox({
                    hbox({
                        text("FTB") | bold | borderDouble | bgcolor(Color::BlueLight) |
                        size(WIDTH, LESS_THAN, 5),
                        filler() | size(WIDTH, EQUAL, 1),
                        text("🤖当前路径: " + displayPath) |
                        bold | borderHeavy | color(Color::Pink1) |
                        size(HEIGHT, LESS_THAN, 1) | flex
                    }),
                    // 马里奥运动动画 - 放在FTB徽章下方，增加高度
                    marioAnimationRenderer() | size(HEIGHT, EQUAL, 14) | size(WIDTH, EQUAL, 40)
                }) | size(WIDTH, EQUAL, 80),
                filler(),
                // 中间：显示当前选中项大小和 % 进度条
                vbox({
                    hbox({
                        text(" █ ") | color(Color::Cyan),
                        text(selected_size)
                    }) | size(WIDTH, LESS_THAN, 25),
                    hbox({
                        text("[") | color(Color::Yellow3),
                        gauge(size_ratio.load()) | flex | color(Color::Green) | size(WIDTH, EQUAL, 20),
                        text("]") | color(Color::Yellow3)
                    }),
                    hbox({
                        text(" ▓ ") | color(Color::Yellow),
                        text(ratio_stream.str() + "%") | bold
                    })
                }) | border | color(Color::Purple3) | size(HEIGHT, EQUAL, 3),
                WeatherDisplay::render(),  // 天气组件 - 实时渲染
                // 右侧：当前时间 + 加载指示符
                vbox({
                    text(time_str) | color(Color::GrayDark),
                    text(loadingIndicator) | color(Color::Green)
                }) | borderDouble | color(Color::Purple3) | size(HEIGHT, EQUAL, 5)
            }),
            // 第二行：搜索框 + 分页信息
            hbox({
                searchInput->Render() | border | color(Color::Magenta) | size(WIDTH, EQUAL, 120),
                filler(),
                page_info
            }),
            // 第三行：网格文件/文件夹列表
            vbox(grid_elements) | color(Color::Blue) | frame | borderDashed |
                color(Color::GrayDark) | flex | yflex,
            // 最底部一行：空进度条作为底部边框
            gauge(1) | color(Color::RGB(158, 160, 161)) | size(WIDTH, EQUAL, 190)
        });
    });

    // ---------- 右侧详细信息面板 & Vim 编辑器模式 ----------

    // detail_width 已在上面从配置中初始化
    bool vim_mode_active = false;        // 是否进入 Vim 模式编辑
    std::unique_ptr<VimLikeEditor> vimEditor = nullptr;  // Vim 编辑器智能指针，初始为空

    // 定义右侧面板的渲染函数：根据是否进入 Vim 模式选择渲染内容
    auto detailElement = Renderer([&]() -> Element {
        if (vim_mode_active && vimEditor != nullptr)
            return vimEditor->Render();  // 渲染 Vim 编辑器
        else
            return CreateDetailElement(filteredContents, selected, currentPath);  // 渲染详情视图
    });

    // 将左侧主面板（renderer）和右侧详情面板（detailElement）进行可调整大小的分割
    auto splitted = ResizableSplitRight(detailElement, renderer, &detail_width);

    // 捕获键盘事件并做相应处理的顶层组件
    auto final_component = CatchEvent(splitted, [&](Event event) {
        // 如果 Vim 模式激活且有编辑器，则先交给编辑器处理按键事件
        if (vim_mode_active && vimEditor != nullptr)
        {
            if (vimEditor->OnEvent(event))
                return true;  // 编辑器已经处理该事件，就返回
        }

        // 分页导航：+ 键翻到下一页，- 键翻到上一页
        if (event == Event::Character('+') && event.is_character())
        {
            if (current_page < total_pages - 1)
            {
                current_page++;
                return true;
            }
        }
        else if (event == Event::Character('-') && event.is_character())
        {
            if (current_page > 0)
            {
                current_page--;
                return true;
            }
        }

        // 使用上下左右键在网格中移动选中项
        if (event == Event::ArrowUp)
        {
            if (selected >= items_per_row)
            {
                selected -= items_per_row;
                // 如果移动后不在当前页，跳转到对应页
                if (selected < current_page * items_per_page)
                {
                    current_page = selected / items_per_page;
                }
                return true;
            }
        }
        else if (event == Event::ArrowDown)
        {
            if (selected + items_per_row < static_cast<int>(filteredContents.size()))
            {
                selected += items_per_row;
                if (selected >= (current_page + 1) * items_per_page)
                {
                    current_page = selected / items_per_page;
                }
                return true;
            }
        }
        else if (event == Event::ArrowLeft)
        {
            if (selected > 0)
            {
                selected--;
                if (selected < current_page * items_per_page)
                {
                    current_page = selected / items_per_page;
                }
                return true;
            }
        }
        else if (event == Event::ArrowRight)
        {
            if (selected + 1 < static_cast<int>(filteredContents.size()))
            {
                selected++;
                if (selected >= (current_page + 1) * items_per_page)
                {
                    current_page = selected / items_per_page;
                }
                return true;
            }
        }


        // 将其他按键事件交给 UIManager 处理（包括回车进入目录、删除返回上级、搜索、Vim 编辑等）
        if (UIManager::handleEvents(event,
                                    directoryHistory,
                                    currentPath,
                                    allContents,
                                    filteredContents,
                                    selected,
                                    searchQuery,
                                    screen,
                                    refresh_ui,
                                    vim_mode_active,
                                    vimEditor))
        {
            // 如果目录发生变化或搜索框按下回车/删除，则重置到第一页
            if (event == Event::Return || event == Event::Backspace || !searchQuery.empty())
            {
                current_page = 0;
            }
            return true;
        }

        // 未处理的事件返回 false
        return false;
    });

    // 启动 FTXUI 主循环，渲染 final_component 并响应事件
    screen.Loop(final_component);

    // 程序结束前清理资源（返回到对象池）
    if (vimEditor) {
        FTB::VimEditorPool::getInstance().release(std::move(vimEditor));
    }
    
    // 停止天气服务
    weather_service->Stop();
    
    // 清理异步文件管理器
    FTB::GlobalAsyncFileManager::cleanup();

    return 0;
}