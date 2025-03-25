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

#include "../include/FTB/ClipboardManager.hpp"
#include "../include/FTB/DirectoryHistory.hpp"
#include "../include/FTB/FileManager.hpp"
#include "../include/FTB/FileSizeCalculator.hpp"
#include "../include/FTB/ThreadGuard.hpp"
#include "../include/FTB/UIManager.hpp"
#include "../include/FTB/Vim_Like.hpp"
#include "../include/FTB/detail_element.hpp"

using namespace ftxui;
namespace fs = std::filesystem;

const std::string FOLDER_ICON = "📁 ";
const std::string FILE_ICON   = "📄 ";

const std::vector<std::string> loadingFrames = {" ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓", "░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ",
                                                "▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░", "▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒",
                                                " ░▒▓ ░▒▓ ░▒▓ ░▒▓ ░▒▓"};

int main()
{
    int                      hovered_index = -1;
    DirectoryHistory         directoryHistory;  // 使用 DirectoryHistory 对象
    std::string              currentPath      = fs::absolute(".").string();
    auto                     allContents      = FileManager::getDirectoryContents(currentPath);
    std::vector<std::string> filteredContents = allContents;
    int                      selected         = 0;
    auto                     screen           = ScreenInteractive::Fullscreen();

    std::atomic<double>    size_ratio(0.0);
    std::atomic<uintmax_t> total_folder_size(0);
    std::string            selected_size;
    std::future<void>      size_future;

    std::atomic<double> wave_progress(0.0);
    std::atomic<bool>   refresh_ui{true};

    std::thread wave_thread([&] {
        while (refresh_ui)
        {
            wave_progress.store(wave_progress.load() + 0.1, std::memory_order_relaxed);
            if (wave_progress.load() > 2 * M_PI)
                wave_progress.store(0.0, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard waveGuard(wave_thread);

    std::thread timer([&] {
        while (refresh_ui)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            screen.Post(Event::Custom);
        }
    });
    ThreadGuard timerGuard(timer);

    std::string searchQuery;
    auto        ui_pair     = UIManager::initializeUI(searchQuery, filteredContents, selected,
                                                      directoryHistory, currentPath, allContents);
    auto&       component   = ui_pair.first;
    auto&       searchInput = ui_pair.second;

    auto calculateSizes = [&]() {
        FileSizeCalculator::CalculateSizes(currentPath, selected, total_folder_size, size_ratio,
                                           selected_size);
    };

    // 快捷键说明
    auto key_info =
        frame(vbox(
            {text("快捷键说明：") | color(Color::Orange4) | bold,
             text("↑/↓ 导航文件列表") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("Enter 进入目录") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("Backspace 返回上级") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("Ctrl+k键 新建文件夹") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("Ctrl+f键 新建文件") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("空格键查看选中文件夹细节") | color(Color::GrayDark) | size(HEIGHT, LESS_THAN, 1),
             text("Delete键 删除文件夹或文件") | color(Color::GrayDark) |
                 size(HEIGHT, LESS_THAN, 1),
             text("Ctrl+p键 指定预览文件的内容") | color(Color::GrayDark) |
                 size(HEIGHT, LESS_THAN, 1),
             text("Ctrl+e键 进入Vim-Like编辑模式") | color(Color::GrayDark) |
                 size(HEIGHT, LESS_THAN, 1),
             text("增加文件复制、剪切、粘贴") | color(Color::GrayDark) | size(HEIGHT,LESS_THAN,1),
             text("ESC 退出程序") | color(Color::Red3) | size(HEIGHT, LESS_THAN, 1)})) |
        vscroll_indicator | border | color(Color::Purple3) | size(WIDTH, LESS_THAN, 35) |
        size(HEIGHT, EQUAL, 15);

    // 波浪线
    auto waveGauge = [&] {
        Elements wave_elements;
        for (int i = 0; i < 10; ++i)
        {
            double progress   = wave_progress.load() + i * 0.4;
            double wave_value = (std::sin(progress) + 1) / 2;
            wave_elements.push_back(gauge(wave_value) | color(Color::BlueLight) |
                                    size(HEIGHT, LESS_THAN, 1));
        }
        return vbox(std::move(wave_elements)) | borderDouble | color(Color::RGB(33, 136, 143)) |
               size(WIDTH, EQUAL, 40) | size(HEIGHT, EQUAL, 10);
    };

    // 渲染函数
    auto renderer = Renderer(component, [&] {
        calculateSizes();
        auto        now      = std::chrono::system_clock::now();
        std::time_t now_c    = std::chrono::system_clock::to_time_t(now);
        std::tm     now_tm   = *std::localtime(&now_c);
        std::string time_str = FileManager::formatTime(now_tm);

        std::string displayPath = fs::canonical(currentPath).string();
        if (displayPath.length() > 40)
        {
            displayPath = "..." + displayPath.substr(displayPath.length() - 37);
        }

        if (!searchQuery.empty())
        {
            filteredContents.clear();
            for (const auto& item : allContents)
            {
                if (item.find(searchQuery) != std::string::npos)
                    filteredContents.push_back(item);
            }
        }
        else
        {
            filteredContents = allContents;
        }

        Elements elements;
        for (size_t i = 0; i < filteredContents.size(); ++i)
        {
            std::string fullPath = currentPath + "/" + filteredContents[i];
            bool        is_dir   = FileManager::isDirectory(fullPath);
            auto        text_color =
                is_dir ? color(Color::RGB(135, 206, 250)) : color(Color::RGB(255, 99, 71));
            Decorator bg_style = nothing;
            if (hovered_index == (int) i)
                bg_style = bgcolor(Color::RGB(120, 120, 120)) | bold;
            if (selected == (int) i)
                bg_style = bgcolor(Color::GrayLight) | color(Color::Black) | bold;
            std::string itemText = filteredContents[i];
            std::string icon     = is_dir ? FOLDER_ICON : FILE_ICON;
            size_t      pos      = itemText.find(searchQuery);
            if (pos != std::string::npos && !searchQuery.empty())
            {
                Elements highlighted;
                if (pos > 0)
                    highlighted.push_back(text(itemText.substr(0, pos)));
                highlighted.push_back(text(itemText.substr(pos, searchQuery.size())) |
                                      color(Color::GrayLight));
                if (pos + searchQuery.size() < itemText.size())
                    highlighted.push_back(text(itemText.substr(pos + searchQuery.size())));
                elements.push_back(hbox({text(selected == (int) i ? "→ " : "  "), text(icon),
                                         hbox(highlighted) | bold | text_color |
                                             (selected == (int) i ? underlined : nothing),
                                         filler()}) |
                                   borderHeavy | bg_style | size(WIDTH, LESS_THAN, 50));
            }
            else
            {
                elements.push_back(hbox({text(selected == (int) i ? "→ " : "  "), text(icon),
                                         text(itemText) | bold | text_color |
                                             (selected == (int) i ? underlined : nothing),
                                         filler()}) |
                                   borderHeavy | bg_style | size(WIDTH, LESS_THAN, 50));
            }
        }

        const int             max_items_per_column = 5;
        std::vector<Elements> columns;
        for (size_t start = 0; start < elements.size(); start += max_items_per_column)
        {
            auto end = std::min(start + max_items_per_column, elements.size());
            columns.emplace_back(elements.begin() + start, elements.begin() + end);
        }
        Elements column_boxes;
        for (auto& col : columns)
            column_boxes.push_back(vbox(std::move(col)) | flex);

        static int  loadingIndex     = 0;
        std::string loadingIndicator = loadingFrames[loadingIndex % loadingFrames.size()];
        loadingIndex                 = (loadingIndex + 1) % (loadingFrames.size() * 2);
        std::ostringstream ratio_stream;
        ratio_stream << std::fixed << std::setprecision(2) << (size_ratio.load() * 100);

        return vbox(
            {hbox({vbox({hbox({text("FTB") | bold | borderDouble | bgcolor(Color::BlueLight) |
                                   size(WIDTH, LESS_THAN, 5),
                               filler() | size(WIDTH, EQUAL, 2),
                               text("🤖当前路径: " + displayPath) | bold | borderHeavy |
                                   color(Color::Pink1) | size(HEIGHT, LESS_THAN, 1) | flex}),
                         waveGauge() | size(HEIGHT, EQUAL, 10) | size(WIDTH, LESS_THAN, 75)}) |
                       size(WIDTH, EQUAL, 80),
                   filler(),
                   vbox({hbox({text(" █ ") | color(Color::Cyan), text(selected_size)}) |
                             size(WIDTH, LESS_THAN, 25),
                         hbox({text("[") | color(Color::Yellow3),
                               gauge(size_ratio.load()) | flex | color(Color::Green) |
                                   size(WIDTH, EQUAL, 20),
                               text("]") | color(Color::Yellow3)}),
                         hbox({text(" ▓ ") | color(Color::Yellow),
                               text(ratio_stream.str() + "%") | bold})}) |
                       border | color(Color::Purple3) | size(HEIGHT, EQUAL, 3),
                   key_info,
                   vbox({text(time_str) | color(Color::GrayDark),
                         text(loadingIndicator) | color(Color::Green)}) |
                       borderDouble | color(Color::Purple3) | size(HEIGHT, EQUAL, 5)}),
             searchInput->Render() | border | color(Color::Magenta) | size(WIDTH, LESS_THAN, 100),
             hbox(column_boxes) | color(Color::Blue) | frame | borderDashed |
                 color(Color::GrayDark) | flex,
             gauge(1) | color(Color::RGB(158, 160, 161)) | size(WIDTH, EQUAL, 190)});
    });

    int detail_width = 25;
    // 定义 vim 编辑模式相关变量
    bool           vim_mode_active = false;
    VimLikeEditor* vimEditor       = nullptr;

    // 右侧区域，根据 vim_mode_active 决定显示内容（详情信息或 Vim 编辑器）
    auto detailElement = Renderer([&]() -> Element {
        if (vim_mode_active && vimEditor != nullptr)
            return vimEditor->Render();
        else
        {
            return CreateDetailElement(filteredContents, selected, currentPath);
        }
    });

    // 构造分割器组件：左侧为文件列表，右侧为详情的Vim 编辑器
    auto splitted = ResizableSplitRight(detailElement, renderer, &detail_width);

    // 事件捕获组件：如果处于 Vim 模式，优先交给 vimEditor 处理
    auto final_component = CatchEvent(splitted, [&](Event event) {
        if (vim_mode_active && vimEditor != nullptr)
        {
            if (vimEditor->OnEvent(event))
                return true;
        }
        if (UIManager::handleEvents(event, directoryHistory, currentPath, allContents,
                                    filteredContents, selected, searchQuery, screen, refresh_ui,
                                    vim_mode_active, vimEditor))
            return true;
        return false;
    });

    screen.Loop(final_component);

    if (vimEditor != nullptr)
        delete vimEditor;
    return 0;
}
