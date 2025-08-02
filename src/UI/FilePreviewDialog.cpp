#include "../../include/UI/FilePreviewDialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <ftxui/component/event.hpp>
#include "../../include/FTB/FileManager.hpp"  // 用于读取文件内容

using namespace ftxui;
namespace fs = std::filesystem;

namespace FilePreviewDialog {

void show(ScreenInteractive& screen, const std::string& fullPath, const std::string& fileContent) {
    int scrollOffset = 0;
    int totalLines = std::count(fileContent.begin(), fileContent.end(), '\n');
    int visibleLines = 10;
    int maxScroll = std::max(0, totalLines - visibleLines);

    auto scrollSlider = Slider("🔄 滚动", &scrollOffset, 0, maxScroll, 1) | color(Color::Orange1);
    auto exitButton = Button("🚪退出", [&] { screen.Exit(); });

    auto renderer = Renderer([&] {
        std::istringstream iss(fileContent);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        std::vector<Element> textElements;
        for (int i = scrollOffset; i < std::min(scrollOffset + visibleLines, (int)lines.size()); ++i) {
            textElements.push_back(text(lines[i]));
        }
        return vbox({
            text("📄 文件内容预览: " + std::string(fs::path(fullPath).filename().string())) 
                | bold | borderDouble | color(Color::Green),
            vbox(textElements) | borderDouble | color(Color::GreenYellow) 
                | size(WIDTH, EQUAL, 150) | size(HEIGHT, EQUAL, 15),
            scrollSlider->Render(),
            exitButton->Render() | borderLight | size(WIDTH, EQUAL, 10) | center,
        }) | center;
    });

    auto eventHandler = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Return) {
            screen.Exit();
            return true;
        }
        if (event.is_mouse()) {
            auto mouse = event.mouse();
            if (mouse.button == Mouse::WheelUp) {
                scrollOffset = std::max(0, scrollOffset - 1);
                return true;
            }
            if (mouse.button == Mouse::WheelDown) {
                scrollOffset = std::min(maxScroll, scrollOffset + 1);
                return true;
            }
        }
        return false;
    });
    screen.Loop(eventHandler);
}

// 新增的交互式输入行数的预览函数
void showWithRangeInput(ScreenInteractive& screen, const std::string& fullPath) {
    std::string inputStart;
    std::string inputEnd;
    bool cancelled = false;
    auto startInput = Input(&inputStart, "🔢 起始行");
    auto endInput = Input(&inputEnd, "🔢 结束行");
    auto confirmButton = Button("✅ 确定", [&] { screen.Exit(); });
    auto cancelButton = Button("❌ 取消", [&] { cancelled = true; screen.Exit(); });
    
    auto container = Container::Vertical({startInput, endInput, confirmButton, cancelButton});
    auto inputUI = Renderer(container, [&] {
        return vbox({
            text("📄 请输入预览的行数范围:") | bold | color(Color::CadetBlue),
            hbox({text("🔢 起始行: ") | color(Color::CadetBlue), startInput->Render()}) | center,
            hbox({text("🔢 结束行: ") | color(Color::CadetBlue), endInput->Render()}) | center,
            hbox({confirmButton->Render() | color(Color::CadetBlue),
                  text(" "),
                  cancelButton->Render() | color(Color::CadetBlue)}) | center,
        }) | borderDouble | color(Color::GrayLight) | center;
    });
    screen.Loop(inputUI);
    if (cancelled)
        return;
    size_t startLine, endLine;
    try {
        startLine = std::stoul(inputStart);
        endLine = std::stoul(inputEnd);
    } catch (...) {
        // 若输入有误，则使用默认值
        startLine = 1;
        endLine = 20;
    }
    // 调用 FileManager 获取指定行数的文件内容
    std::string fileContent = FileManager::readFileContent(fullPath, startLine, endLine);
    show(screen, fullPath, fileContent);
}

} // namespace FilePreviewDialog
