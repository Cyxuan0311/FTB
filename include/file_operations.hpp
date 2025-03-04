#pragma once
#include <stack>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <regex>
#include "../include/file_browser.hpp"

namespace fs = std::filesystem;

// 定义DirectoryCache结构体
struct DirectoryCache {
    std::vector<std::string> contents;
    std::chrono::system_clock::time_point last_update;
    bool valid = false;
};

// 全局缓存变量声明
extern std::mutex cache_mutex;
extern std::unordered_map<std::string, DirectoryCache> dir_cache;

// 进入目录函数
void enterDirectory(std::stack<std::string>& pathHistory, 
                    std::string& currentPath,
                    std::vector<std::string>& contents,
                    int& selected);

namespace FileOperations {
    // 验证名称合法性：加上 inline 关键字避免多重定义问题
    inline bool isValidName(const std::string& name) {
        std::regex pattern("^[a-zA-Z0-9_.-]+$");
        return std::regex_match(name, pattern);
    }

    // 创建文件
    bool createFile(const std::string& filePath);

    // 创建目录
    bool createDirectory(const std::string& dirPath);

    // 删除文件或目录
    bool deleteFileOrDirectory(const std::string& path);
}
