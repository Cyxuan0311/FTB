#include "../include/FTB/FileManager.hpp"
#include "../include/FTB/DirectoryHistory.hpp"
#include <filesystem>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <numeric>
#include <algorithm>

namespace fs = std::filesystem;

namespace FileManager {

// 全局变量定义，使用 std::map
std::mutex cache_mutex;
std::map<std::string, DirectoryCache> dir_cache;
std::map<std::string, FileChunkCache> fileChunkCache;

bool isDirectory(const std::string & path) {
    struct stat statbuf;
    return stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}
// 获取目录内容
std::vector<std::string> getDirectoryContents(const std::string & path) {
    std::vector<std::string> contents;
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name != "." && name != "..") {
                contents.push_back(name);
            }
        }
        closedir(dir);
    }
    return contents;
}
// 格式化时间
std::string formatTime(const std::tm & time) {
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
    return buffer;
}
// 计算目录大小
uintmax_t calculateDirectorySize(const std::string & path) {
    uintmax_t total = 0;
    try {
        for (const auto & entry : fs::recursive_directory_iterator(path)) {
            if (!fs::is_directory(entry))
                total += fs::file_size(entry);
        }
    } catch (...) {
        return 0;
    }
    return total;
}
// 获取文件大小
uintmax_t getFileSize(const std::string & path) {
    try {
        if (isDirectory(path))
            return calculateDirectorySize(path);
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}
// 验证名称
bool isValidName(const std::string & name) {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
        return false;
    return true;
}
// 创建文件
bool createFile(const std::string & filePath) {
    std::string filename = fs::path(filePath).filename().string();
    if (!isValidName(filename)) {
        std::cerr << "Invalid file name: " << filePath << std::endl;
        return false;
    }
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to create file " << filePath << ". Error: " 
                  << std::strerror(errno) << std::endl;
        return false;
    }
    file.close();
    return true;
}
// 创建目录
bool createDirectory(const std::string & dirPath) {
    std::string dirname = fs::path(dirPath).filename().string();
    if (!isValidName(dirname)) {
        std::cerr << "Invalid directory name: " << dirPath << std::endl;
        return false;
    }
    if (!fs::create_directory(dirPath)) {
        std::cerr << "Failed to create directory " << dirPath 
                  << ". Error: " << std::strerror(errno) << std::endl;
        return false;
    }
    std::string parentPath = fs::path(dirPath).parent_path().string();
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        if (dir_cache.count(dirPath))
            dir_cache.erase(dirPath);
        if (dir_cache.count(parentPath))
            dir_cache[parentPath].valid = false;
    }
    return true;
}
// 删除文件或目录
bool deleteFileOrDirectory(const std::string & path) {
    try {
        if (fs::is_directory(path))
            return fs::remove_all(path) > 0;
        else if (fs::is_regular_file(path))
            return fs::remove(path);
    } catch (...) {
        return false;
    }
    return false;
}
// 检查是否为禁止文件
bool isForbiddenFile(const std::string & filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = filePath.substr(dotPos);
        constexpr const char* forbiddenExtensions[] = {".a", ".o", ".bin", ".exe", ".dll", ".so", ".dat", ".tmp", ".swp"};
        for (const auto& forbiddenExt : forbiddenExtensions) {
            if (ext == forbiddenExt)
                return true;
        }
    }
    return false;
}

// 进入目录函数：利用 std::map 存储的缓存
void enterDirectory(DirectoryHistory & history,
                    std::string & currentPath,
                    std::vector<std::string> & contents,
                    int & selected) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (!fs::exists(currentPath) || !fs::is_directory(currentPath)) {
        std::cerr << "Invalid working directory: " << currentPath << std::endl;
        selected = -1;
        return;
    }
    auto &cache = dir_cache[currentPath];
    if (!cache.valid) {
        cache.contents = getDirectoryContents(currentPath);
        cache.valid = true;
        cache.last_update = std::chrono::system_clock::now();
    }
    bool valid_selection = !cache.contents.empty() && (selected >= 0) &&
                           (static_cast<size_t>(selected) < cache.contents.size());
    if (!valid_selection) {
        selected = -1;
        return;
    }
    fs::path fullPath = fs::path(currentPath) / cache.contents[selected];
    if (!fs::exists(fullPath) || !isDirectory(fullPath.string())) {
        std::cerr << "Target is not a valid directory: " << fullPath << std::endl;
        selected = -1;
        return;
    }
    auto &new_cache = dir_cache[fullPath.string()];
    new_cache.contents = getDirectoryContents(fullPath.string());
    new_cache.valid = true;
    new_cache.last_update = std::chrono::system_clock::now();
    history.push(currentPath);
    currentPath = fullPath.lexically_normal().string();
    contents = new_cache.contents;
    selected = contents.empty() ? -1 : 0;
}
// 计算当前文件夹文件数量和权限
void calculation_current_folder_files_number(
    const std::string & path,
    int & file_count,
    int & folder_count,
    std::vector<std::tuple<std::string, mode_t>> & folder_permissions,
    std::vector<std::string> & fileNames) {
    file_count = 0;
    folder_count = 0;
    folder_permissions.clear();
    fileNames.clear();
    try {
        for (const auto &entry : fs::directory_iterator(path)) {
            std::string name = entry.path().filename().string();
            fileNames.push_back(name);
            if (entry.is_directory()) {
                folder_count++;
                struct stat fileStat;
                if (stat(entry.path().c_str(), &fileStat) == 0) {
                    folder_permissions.emplace_back(name, fileStat.st_mode);
                } else {
                    std::cerr << "Error getting permissions for " << entry.path() << std::endl;
                }
            } else if (entry.is_regular_file()) {
                file_count++;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error counting entries in " << path << ": " << e.what() << std::endl;
    }
}
// 读取文件内容
std::string readFileContent(const std::string & filePath, size_t startLine, size_t endLine) {
    const std::chrono::seconds expiry(60); // 缓存有效期 60 秒

    // **强制刷新缓存：如果 `writeFileContent` 修改了文件，就重新读取**
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = fileChunkCache.find(filePath);
    if (it != fileChunkCache.end()) {
        auto now = std::chrono::system_clock::now();
        if (now - it->second.last_update < expiry) {
            std::cerr << "[DEBUG] Using cached content for: " << filePath << std::endl;
        } else {
            //std::cerr << "[DEBUG] Cache expired for: " << filePath << ", reloading..." << std::endl;
            fileChunkCache.erase(it);
        }
    }

    // **从磁盘强制读取最新数据**
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filePath << std::endl;
        return "错误: 无法打开文件";
    }

    std::string line, result;
    size_t currentLine = 0;
    while (std::getline(file, line)) {
        currentLine++;
        if (currentLine >= startLine && currentLine <= endLine) {
            result += line + "\n";
        }
        if (currentLine > endLine) break;
    }
    file.close();

    // **更新缓存**
    fileChunkCache[filePath] = FileChunkCache{{ {0, result} }, std::chrono::system_clock::now()};
    return result;
}
// 写入文件内容
bool writeFileContent(const std::string & filePath, const std::string & content) {
    std::ofstream ofs(filePath, std::ios::trunc);  // 覆盖写入
    if (!ofs) {
        std::cerr << "[ERROR] Cannot open file for writing: " << filePath << std::endl;
        return false;
    }
    ofs << content;
    ofs.close();

    // **清除缓存，确保下次读取新数据**
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        if (fileChunkCache.count(filePath)) {
            fileChunkCache.erase(filePath);
            //std::cerr << "[DEBUG] Cache cleared for file: " << filePath << std::endl;
        }

        std::string parentDir = fs::path(filePath).parent_path().string();
        if (dir_cache.count(parentDir)) {
            dir_cache[parentDir].valid = false;
            //std::cerr << "[DEBUG] Cache cleared for directory: " << parentDir << std::endl;
        }
    }

    return true;
}

// 清理过期缓存
void clearFileChunkCache(const std::chrono::seconds& expiry) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto now = std::chrono::system_clock::now();
    for (auto it = fileChunkCache.begin(); it != fileChunkCache.end(); ) {
        if (now - it->second.last_update > expiry)
            it = fileChunkCache.erase(it);
        else
            ++it;
    }
}


} // namespace FileManager

//Google Benchmark、perf使用