#include "../../include/UI/NewFolderDialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace NewFolderDialog {

std::string show(ScreenInteractive& screen) {
    std::string dirName;
    bool isConfirmed = false; // 新增：用于标记用户是否确认创建

    auto dirNameInput = Input(&dirName, "📂 文件夹名");
    auto cancelButton = Button("❌ 取消", [&] { screen.Exit(); });
    auto createButton = Button("✅ 创建", [&] {
        isConfirmed = true; // 标记用户确认创建
        screen.Exit();
    });

    auto container = Container::Vertical({
        dirNameInput,
        Container::Horizontal({cancelButton, createButton})
    });
    auto renderer = Renderer(container, [&] {
        return vbox({
            text("📁 新建文件夹") | bgcolor(Color::Orange4Bis),
            dirNameInput->Render(),
            hbox({cancelButton->Render() | color(Color::Orange4Bis), filler(),
                  createButton->Render() | color(Color::Orange4Bis)}) | hcenter
        }) | borderDouble | color(Color::GrayLight) | size(WIDTH, GREATER_THAN, 50) | vcenter | hcenter;
    });
    screen.Loop(renderer);

    // 仅在用户确认创建时返回文件夹名
    if (isConfirmed && !dirName.empty())
        return dirName;
    return ""; // 取消时返回空字符串
}

} // namespace NewFolderDialog