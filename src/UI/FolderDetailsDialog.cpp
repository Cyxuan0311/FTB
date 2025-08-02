#include "../../include/UI/FolderDetailsDialog.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace FolderDetailsDialog {

// 渲染文件夹信息的函数
std::vector<Element> renderFolderInfo(const FolderDetails& details) {
    std::vector<Element> elements;
    // 标题
    elements.push_back(text("📂 文件夹详情") | bold | borderHeavy | center | color(Color::Green3));
    // 路径信息
    elements.push_back(hbox({text("📍 路径: "), text(details.folderPath) | underlined | color(Color::Orange1)}));
    // 文件夹和文件数量
    elements.push_back(hbox({text("📁 文件夹数: "), text(std::to_string(details.folderCount)) | color(Color::Orange1)}));
    elements.push_back(hbox({text("📄 文件数: "), text(std::to_string(details.fileCount)) | color(Color::Orange1)}));
    elements.push_back(separator());
    // 文件/文件夹列表
    elements.push_back(text("📌 文件/文件夹列表:") | bold | color(Color::BlueLight));
    for (const auto& name : details.fileNames) {
        bool isDir = details.permissions.find(name) != details.permissions.end();
        elements.push_back(hbox({text(isDir ? "📂 " : "📄 ") | color(Color::Yellow1),
            text(name) | color(Color::White)}));
    }
    elements.push_back(separator());
    // 权限信息
    elements.push_back(text("🛡 文件夹权限信息:") | bold | color(Color::BlueLight));
    for (const auto& perm : details.permissions) {
        elements.push_back(hbox({text("📂 ") | bold | color(Color::Yellow1), text(perm.first) | underlined}));
        elements.push_back(hbox({text("   🛡 权限: ") | bold | color(Color::GrayLight), text(std::to_string(perm.second)) | color(Color::Cyan)}));
    }
    return elements;
}

void show(ScreenInteractive& screen, const FolderDetails& details) {
    auto exitButton = Button("🚪 退出", [&] { screen.Exit(); });
    auto renderer = Renderer([&] {
        std::vector<Element> elements = renderFolderInfo(details);
        elements.push_back(hbox({filler(), exitButton->Render(), filler()}) | center);
        elements.push_back(text("🚪 按 ENTER 退出") | bold | color(Color::Red3Bis));
        return vbox(elements) | borderDouble | center | color(Color::RGB(185,185,168));
    });

    // 监听Enter键
    renderer |= CatchEvent([&](Event event) {
        if (event == Event::Character('\n')) { // Enter键
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(renderer);
}

} // namespace FolderDetailsDialog