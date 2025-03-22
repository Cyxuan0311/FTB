#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <tuple>
#include "DirectoryHistory.hpp"
#include <sys/stat.h>

// 新增：定义分块大小
constexpr size_t CHUNK_SIZE = 8192; // 8KB

namespace FileManager {

// 用于缓存目录数据的结构体（保持不变）
struct DirectoryCache {
    bool valid = false;
    std::vector<std::string> contents;
    std::vector<uintmax_t> sizes;
    uintmax_t total_size = 0;
    std::chrono::system_clock::time_point last_update;
};

// 用于缓存文件分块数据的结构体
struct FileChunkCache {
    // 使用 map 存储块数据，键为块索引
    std::map<size_t, std::string> chunks;
    std::chrono::system_clock::time_point last_update;
};

// 全局缓存，使用 std::map 存储
extern std::mutex cache_mutex;
extern std::map<std::string, DirectoryCache> dir_cache;
extern std::map<std::string, FileChunkCache> fileChunkCache;

// 原有接口声明
bool isDirectory(const std::string & path);
std::vector<std::string> getDirectoryContents(const std::string & path);
std::string formatTime(const std::tm & time);
uintmax_t calculateDirectorySize(const std::string & path);
uintmax_t getFileSize(const std::string & path);
bool isValidName(const std::string & name);
bool createFile(const std::string & filePath);
bool createDirectory(const std::string & dirPath);
bool deleteFileOrDirectory(const std::string & path);
bool isForbiddenFile(const std::string & filePath);
void enterDirectory(DirectoryHistory & history,
                    std::string & currentPath,
                    std::vector<std::string> & contents,
                    int & selected);
void calculation_current_folder_files_number(
    const std::string & path,
    int & file_count,
    int & folder_count,
    std::vector<std::tuple<std::string, mode_t>> & folder_permissions,
    std::vector<std::string> & fileNames);

// 修改后的文件读取接口，使用分块加载
std::string readFileContent(const std::string & filePath, size_t startLine, size_t endLine);
bool writeFileContent(const std::string & filePath, const std::string & content);

// 新增：清理过期的文件块缓存接口
void clearFileChunkCache(const std::chrono::seconds& expiry);

} // namespace FileManager

#endif // FILE_MANAGER_HPP
