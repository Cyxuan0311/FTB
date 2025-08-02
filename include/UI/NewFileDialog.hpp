#ifndef NEW_FILE_DIALOG_HPP
#define NEW_FILE_DIALOG_HPP

#include <string>
#include <ftxui/component/screen_interactive.hpp>

namespace NewFileDialog {
    // 显示新建文件对话框，返回构造后的文件名（例如 "example.txt"），如果取消则返回空字符串
    std::string show(ftxui::ScreenInteractive& screen);
}

#endif // NEW_FILE_DIALOG_HPP
