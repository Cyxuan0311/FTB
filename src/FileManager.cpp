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
// 读取文件内容，使用分块加载
std::string readFileContent(const std::string & filePath, size_t startLine, size_t endLine) {
    const std::chrono::seconds expiry(60); // 缓存有效期60秒

    // 尝试从缓存中查找文件分块数据
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = fileChunkCache.find(filePath);
        if (it != fileChunkCache.end()) {
            auto now = std::chrono::system_clock::now();
            if (now - it->second.last_update < expiry) {
                // 缓存命中，合并所有块数据
                std::ostringstream oss;
                for (const auto &chunkPair : it->second.chunks) {
                    oss << chunkPair.second;
                }
                std::string fullContent = oss.str();
                // 提取指定行
                std::istringstream iss(fullContent);
                std::string line, result;
                size_t lineCount = 0;
                while (std::getline(iss, line)) {
                    lineCount++;
                    if (lineCount >= startLine && lineCount <= endLine)
                        result += line + "\n";
                    if (lineCount > endLine)
                        break;
                }
                return result;
            }
        }
    }

    // 缓存中不存在或已过期，进行分块读取
    if (isForbiddenFile(filePath))
        return "错误: 禁止预览此类型的文件。";
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return "错误: 无法打开文件。";

    // 使用 map 存储每个块数据，键为块编号（0开始）
    std::map<size_t, std::string> chunks;
    size_t chunkIndex = 0;
    char buffer[CHUNK_SIZE];

    file.seekg(0, std::ios::beg);
    while (file.read(buffer, CHUNK_SIZE) || file.gcount() > 0) {
        chunks[chunkIndex++] = std::string(buffer, file.gcount());
    }
    file.close();

    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        fileChunkCache[filePath] = FileChunkCache{chunks, std::chrono::system_clock::now()};
    }

    // 合并所有块数据
    std::ostringstream oss;
    for (const auto &chunkPair : chunks) {
        oss << chunkPair.second;
    }
    std::string fullContent = oss.str();

    // 提取需要返回的行
    std::istringstream iss(fullContent);
    std::string line, result;
    size_t lineCount = 0;
    while (std::getline(iss, line)) {
        lineCount++;
        if (lineCount >= startLine && lineCount <= endLine)
            result += line + "\n";
        if (lineCount > endLine)
            break;
    }
    return result;
}

// 写入文件内容
bool writeFileContent(const std::string & filePath, const std::string & content) {
    std::ofstream ofs(filePath);
    if (!ofs) {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
        return false;
    }
    ofs << content;
    ofs.close();
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