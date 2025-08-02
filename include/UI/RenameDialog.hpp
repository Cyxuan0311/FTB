#ifndef RENAME_DIALOG_HPP
#define RENAME_DIALOG_HPP

#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace RenameDialog {
    // 显示重命名对话框，参数 currentName 为当前名称，
    // 返回用户输入的新名称（如果取消则返回空字符串）
    std::string show(ftxui::ScreenInteractive& screen, const std::string& currentName);
}

#endif // RENAME_DIALOG_HPP
