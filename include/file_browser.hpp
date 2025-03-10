#pragma once
#include <string>
#include <vector>
#include <ctime>  // 添加ctime头文件

namespace FileBrowser {
    // 添加函数声明
    bool isDirectory(const std::string& path);
    std::vector<std::string> getDirectoryContents(const std::string& path);
    std::string formatTime(const std::tm& time);  // 添加返回类型声明
    uintmax_t getFileSize(const std::string& path);
    uintmax_t calculateDirectorySize(const std::string& path);
}