#include "../include/file_operations.hpp"
#include <fstream>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

// 定义全局缓存变量
std::mutex cache_mutex;
std::unordered_map<std::string, DirectoryCache> dir_cache;

void enterDirectory(std::stack<std::string>& pathHistory, std::string& currentPath,
    std::vector<std::string>& contents,
    int& selected) 
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // 新增1：严格验证当前路径合法性
    if (!fs::exists(currentPath) || !fs::is_directory(currentPath)) {
        std::cerr << "非法工作路径: " << currentPath << std::endl;
        selected = -1;
        return;
    }

    auto& cache = dir_cache[currentPath];
    if (!cache.valid) {
        cache.contents = FileBrowser::getDirectoryContents(currentPath);
        cache.valid = true;
    }

    // 新增2：加强索引范围验证
    const bool valid_selection = !cache.contents.empty() && 
                               (selected >= 0) && 
                               (static_cast<size_t>(selected) < cache.contents.size());
    if (!valid_selection) {
        selected = -1;
        return;
    }

    auto fullPath = fs::path(currentPath) / cache.contents[selected];
    
    // 新增3：路径双重验证
    if (!fs::exists(fullPath) || !FileBrowser::isDirectory(fullPath.string())) {
        std::cerr << "目标不是有效目录: " << fullPath << std::endl;
        selected = -1;
        return;
    }

    // 预加载目录缓存（强制更新模式）
    auto& new_cache = dir_cache[fullPath.string()];
    new_cache.contents = FileBrowser::getDirectoryContents(fullPath.string()); // 强制刷新
    new_cache.valid = true;

    pathHistory.push(currentPath);
    currentPath = fullPath.lexically_normal().string();
    contents = new_cache.contents;
    
    // 最终安全设置
    selected = contents.empty() ? -1 : 0;
}


namespace FileOperations {
    // 创建文件
    bool createFile(const std::string& filePath) {
        if (!isValidName(fs::path(filePath).filename().string())) {
            std::cerr << "Invalid file name: " << filePath << std::endl;
            return false;
        }
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to create file " << filePath << ". Error: " << std::strerror(errno) << std::endl;
            return false;
        }
        file.close();
        return true;
    }

    // 创建目录
    bool createDirectory(const std::string& dirPath) {
        if (!isValidName(fs::path(dirPath).filename().string())) {
            std::cerr << "Invalid directory name: " << dirPath << std::endl;
            return false;
        }
        if (!fs::create_directory(dirPath)) {
            std::cerr << "Failed to create directory " << dirPath << ". Error: " << std::strerror(errno) << std::endl;
            return false;
        }
        auto parentPath = fs::path(dirPath).parent_path().string();
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            if (dir_cache.count(dirPath)) {
                dir_cache.erase(dirPath);
            }
            // 原有父目录缓存失效逻辑
            if (dir_cache.count(parentPath)) {
                dir_cache[parentPath].valid = false;
            }
        }
        return true;
    }

    // 删除文件或目录
    bool deleteFileOrDirectory(const std::string& path) {
        if (fs::is_directory(path)) {
            return fs::remove_all(path);
        } else if (fs::is_regular_file(path)) {
            return fs::remove(path);
        }
        return false;
    }
}