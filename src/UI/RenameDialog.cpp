#include "../../include/UI/RenameDialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace RenameDialog {

std::string show(ScreenInteractive& screen, const std::string& currentName) {
    std::string newName = currentName;
    auto nameInput = Input(&newName, "😎 新名称");
    bool confirmed = false;
    auto cancelButton = Button("❌ 取消", [&] { screen.Exit(); });
    auto renameButton = Button("✅ 重命名", [&] {
        if (!newName.empty() && newName != currentName)
            confirmed = true;
        screen.Exit();
    });

    auto container = Container::Vertical({
        nameInput,
        Container::Horizontal({cancelButton, renameButton})
    });
    auto renderer = Renderer(container, [&] {
        return vbox({
            text("📝 重命名") | bgcolor(Color::GreenLight),
            nameInput->Render(),
            hbox({
                cancelButton->Render() | color(Color::Red),
                filler(),
                renameButton->Render() | color(Color::Green)
            }) | hcenter
        }) | borderDouble | color(Color::GrayLight) | size(WIDTH, GREATER_THAN, 50) | vcenter | hcenter;
    });
    screen.Loop(renderer);
    return confirmed ? newName : "";
}

} // namespace RenameDialog
