#include "../../include/UI/NewFileDialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace NewFileDialog {

std::string show(ScreenInteractive& screen) {
    std::string fileName;
    std::string fileType;
    bool isConfirmed = false; // 新增：用于标记用户是否确认创建

    auto fileNameInput = Input(&fileName, "📝 文件名");
    auto fileTypeInput = Input(&fileType, "🔤 文件类型");
    auto cancelButton  = Button("❌ 取消", [&] { screen.Exit(); });
    auto createButton  = Button("✅ 创建", [&] {
        isConfirmed = true; // 标记用户确认创建
        screen.Exit();
    });

    auto container = Container::Vertical({
        fileNameInput,
        fileTypeInput,
        Container::Horizontal({cancelButton, createButton})
    });
    auto renderer = Renderer(container, [&] {
        return vbox({
            text("🆕 新建文件") | bgcolor(Color::Green3Bis),
            fileNameInput->Render(),
            fileTypeInput->Render(),
            hbox({filler(), cancelButton->Render() | color(Color::Green3Bis),
                  createButton->Render() | color(Color::Green3Bis), filler()})
                | size(WIDTH, GREATER_THAN, 30)
        }) | borderDouble | color(Color::GrayLight) | size(WIDTH, GREATER_THAN, 50) | vcenter | hcenter;
    });
    screen.Loop(renderer);

    // 仅在用户确认创建时返回文件名
    if (isConfirmed && !fileName.empty() && !fileType.empty())
        return fileName + "." + fileType;
    return ""; // 取消时返回空字符串
}

} // namespace NewFileDialog