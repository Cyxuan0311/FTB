#pragma once

#include <filesystem>
#include <string>

namespace FTB {
namespace Icons {

/**
 * @brief 获取文件夹对应的 Nerd Font 图标。
 * @param path 目标文件夹路径。
 * @return 对应的图标字符串（包含结尾空格，便于直接拼接）。
 */
std::string GetFolderIcon(const std::filesystem::path& path);

/**
 * @brief 获取文件对应的 Nerd Font 图标。
 * @param path 目标文件路径。
 * @return 对应的图标字符串（包含结尾空格，便于直接拼接）。
 */
std::string GetFileIcon(const std::filesystem::path& path);

/**
 * @brief 根据路径类型返回合适的图标。
 * @param path 文件或文件夹路径。
 * @param is_directory 是否为文件夹。
 * @return Nerd Font 图标。
 */
std::string GetIconForPath(const std::filesystem::path& path, bool is_directory);

}  // namespace Icons
}  // namespace FTB


