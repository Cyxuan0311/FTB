#ifndef FILE_PREVIEW_DIALOG_HPP
#define FILE_PREVIEW_DIALOG_HPP

#include <string>
#include <ftxui/component/screen_interactive.hpp>

namespace FilePreviewDialog {

    /**
     * @brief 直接显示文件预览界面
     * @param screen  FTXUI 的屏幕交互对象
     * @param fullPath 文件的完整路径
     * @param fileContent 读取到的文件内容
     */
    void show(ftxui::ScreenInteractive& screen, const std::string& fullPath, const std::string& fileContent);

    /**
     * @brief 交互式输入行号后再显示文件预览界面
     * @param screen FTXUI 的屏幕交互对象
     * @param fullPath 文件的完整路径
     */
    void showWithRangeInput(ftxui::ScreenInteractive& screen, const std::string& fullPath);

} // namespace FilePreviewDialog

#endif // FILE_PREVIEW_DIALOG_HPP
