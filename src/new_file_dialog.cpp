// new_file_dialog.cpp
#include "../include/new_file_dialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>

using namespace ftxui;

std::string showNewFileDialog(ScreenInteractive& screen) {
    std::string fileName;
    std::string fileType;

    auto fileNameInput = Input(&fileName, "文件名");
    auto fileTypeInput = Input(&fileType, "文件类型");

    auto cancelButton = Button("取消", [&] {
        screen.Exit();
    });

    auto createButton = Button("创建", [&] {
        screen.Exit();
    });

    auto container = Container::Vertical({
        fileNameInput,
        fileTypeInput,
        Container::Horizontal({
            cancelButton,
            createButton
        })
    });

    auto renderer = Renderer(container, [&] {
        return vbox({
            text("新建文件"),
            fileNameInput->Render(),
            fileTypeInput->Render(),
            hbox({
                filler(),
                cancelButton->Render(),
                createButton->Render(),
                filler() 
            }) | size(WIDTH, GREATER_THAN, 30)  // 统一按钮容器宽度
        }) | borderRounded | color(Color::Green3Bis) 
          | size(WIDTH, GREATER_THAN, 40)      // 最小宽度限制
          | vcenter | hcenter;                // 分离垂直/水平居中
    });

    screen.Loop(renderer);

    if (!fileName.empty() && !fileType.empty()) {
        return fileName + "." + fileType;
    }
    return "";
}