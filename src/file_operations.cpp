#include "../include/file_operations.hpp"
#include <fstream>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

// 定义全局缓存变量
std::mutex cache_mutex;
std::unordered_map<std::string, DirectoryCache> dir_cache;

void enterDirectory(std::stack<std::string>& pathHistory,
                    std::string& currentPath,
                    std::vector<std::string>& contents,
                    int& selected) 
{
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