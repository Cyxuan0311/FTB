#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <stack>

// 用于缓存目录内容结构体
struct DirectoryCache {
    std::vector<std::string> contents;
    bool valid = false;
    std::chrono::system_clock::time_point last_update;
};

namespace FileManager {
    // 文件浏览相关接口
    bool isDirectory(const std::string & path);
    std::vector<std::string> getDirectoryContents(const std::string & path);
    std::string formatTime(const std::tm & time);
    uintmax_t getFileSize(const std::string & path);
    uintmax_t calculateDirectorySize(const std::string & path);

    // 文件操作接口
    bool createFile(const std::string & filePath);
    bool createDirectory(const std::string & dirPath);
    bool deleteFileOrDirectory(const std::string & path);

    // 目录导航接口
    void enterDirectory(std::stack<std::string>& pathHistory,
                        std::string& currentPath,
                        std::vector<std::string>& contents,
                        int& selected);

    // 名称合法性检查
    bool isValidName(const std::string & name);

    // 全局缓存和互斥锁（在 .cpp 中定义）
    extern std::mutex cache_mutex;
    extern std::unordered_map<std::string, DirectoryCache> dir_cache;
}

#endif
