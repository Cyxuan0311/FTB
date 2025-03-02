#include "../include/file_operations.hpp"
#include <mutex>          // 添加mutex头文件
#include <unordered_map>  // 添加unordered_map头文件
#include <filesystem>     // 添加filesystem头文件
#include <chrono>

namespace fs = std::filesystem;  // 定义filesystem别名

// 定义全局缓存变量
std::mutex cache_mutex;
std::unordered_map<std::string, DirectoryCache> dir_cache;

void enterDirectory(std::stack<std::string>& pathHistory,
                   std::string& currentPath,
                   std::vector<std::string>& contents,
                   int& selected) 
{
    // ... 原有函数体保持不变 ...
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto& cache = dir_cache[currentPath];
    
    if (!cache.valid) {
        cache.contents = FileBrowser::getDirectoryContents(currentPath);
        cache.last_update = std::chrono::system_clock::now();
        cache.valid = true;
    }
    
    if (!cache.contents.empty() && selected < cache.contents.size()) {
        auto fullPath = fs::path(currentPath) / cache.contents[selected];
        if (FileBrowser::isDirectory(fullPath.string())) {
            pathHistory.push(currentPath);
            currentPath = fullPath.lexically_normal().string();
            contents = cache.contents;
            selected = 0;
        }
    }
}