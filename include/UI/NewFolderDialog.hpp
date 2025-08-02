#ifndef NEW_FOLDER_DIALOG_HPP
#define NEW_FOLDER_DIALOG_HPP

#include <string>
#include <ftxui/component/screen_interactive.hpp>

namespace NewFolderDialog {
    // 显示新建文件夹对话框，返回输入的文件夹名称，如果取消则返回空字符串
    std::string show(ftxui::ScreenInteractive& screen);
}

#endif // NEW_FOLDER_DIALOG_HPP
