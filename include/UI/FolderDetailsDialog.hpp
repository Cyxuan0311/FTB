#ifndef FOLDERDETAILSDIALOG_HPP
#define FOLDERDETAILSDIALOG_HPP

#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <string>
#include <vector>
#include <map>

namespace FolderDetailsDialog {

// 文件夹详情结构体
struct FolderDetails {
    std::string folderPath; // 文件夹路径
    int folderCount;        // 文件夹数量
    int fileCount;          // 文件数量
    std::vector<std::string> fileNames; // 文件/文件夹名称列表
    std::map<std::string, mode_t> permissions; // 文件夹权限信息
};

/**
 * @brief 显示文件夹详情对话框
 * @param screen 交互式屏幕对象
 * @param details 文件夹详情信息
 */
void show(ftxui::ScreenInteractive& screen, const FolderDetails& details);

} // namespace FolderDetailsDialog

#endif // FOLDERDETAILSDIALOG_HPP