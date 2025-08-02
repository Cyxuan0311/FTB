// FileManager.cpp
#include "../include/FTB/FileManager.hpp"    // 引入 FileManager 头文件，包含相关声明和类型定义
#include "../include/FTB/DirectoryHistory.hpp"// 引入 DirectoryHistory，用于记录目录历史
#include <filesystem>                         // C++17 文件系统库，用于路径操作和遍历
#include <fstream>                            // 文件读写
#include <iostream>                           // 标准输出
#include <mutex>                              // 互斥锁
#include <map>                                // 字典容器，用于缓存
#include <vector>                             // 动态数组
#include <chrono>                             // 时间操作
#include <cstring>                            // C 字符串操作
#include <numeric>                            // 数值算法
#include <algorithm>                          // 常用算法
#include <system_error>                       // 系统错误处理

namespace fs = std::filesystem;               // 为 std::filesystem 设置简短别名

namespace FileManager {

// ---------------------------- 全局缓存变量 ----------------------------
// 用于保护缓存操作的互斥锁
std::mutex cache_mutex;
// 目录缓存：键为目录路径，值为对应的目录缓存结构
std::map<std::string, DirectoryCache> dir_cache;
// 文件内容缓存：键为文件路径，值为对应的文件内容块缓存
std::map<std::string, FileChunkCache> fileChunkCache;

// ---------------------------- 辅助函数 ----------------------------

/**
 * 判断给定路径是否为目录
 * @param path 要检查的路径
 * @return 如果是目录，则返回 true；否则返回 false
 */
bool isDirectory(const std::string & path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

/**
 * 获取指定目录下的所有文件和子目录名称（不包括 "." 和 ".."）
 * @param path 要读取的目录路径
 * @return 返回包含目录中所有条目名称的字符串向量
 */
std::vector<std::string> getDirectoryContents(const std::string & path) {
    std::vector<std::string> contents;  // 存储结果的字符串向量
    
    try {
        // 使用 C++17 文件系统库遍历目录
        for (const auto& entry : fs::directory_iterator(path)) {
            // 获取当前条目的文件名/目录名
            std::string name = entry.path().filename().string();
            
            // 过滤掉 "." 和 ".." 这两个特殊目录
            if (name != "." && name != "..")
                contents.push_back(name);  // 将有效名称添加到结果向量中
        }
    } catch (const std::exception &e) {
        // 捕获并处理遍历过程中可能发生的异常
        std::cerr << "Error reading directory " << path << ": " << e.what() << std::endl;
    }
    
    return contents;  // 返回包含所有条目名称的向量
}


/**
 * 将 std::tm 结构化时间格式化为 "YYYY-MM-DD HH:MM:SS" 字符串
 * @param time 待格式化的时间结构体
 * @return 格式化后的时间字符串
 */
std::string formatTime(const std::tm & time) {
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
    return buffer;
}

/**
 * 递归计算目录及其所有子目录中文件的总大小
 * @param path 要计算大小的目录路径
 * @return 返回目录中所有文件的总字节数，如果发生错误则返回0
 */
uintmax_t calculateDirectorySize(const std::string & path) {
    uintmax_t total = 0;  // 初始化总大小为0
    
    try {
        // 使用递归目录迭代器遍历目录及其所有子目录
        for (const auto & entry : fs::recursive_directory_iterator(path)) {
            // 只统计文件的大小，跳过目录项
            if (!fs::is_directory(entry)) {
                total += fs::file_size(entry);  // 累加文件大小到总和中
            }
        }
    } catch (...) {
        // 捕获所有可能的异常（如权限不足、路径不存在等）
        return 0;  // 发生错误时返回0
    }
    
    return total;  // 返回计算得到的总大小
}


/**
 * 获取文件或目录的大小
 * @param path 文件或目录路径
 * @return 如果是目录，则返回目录总大小；如果是文件，则返回文件大小；出错返回 0
 */
uintmax_t getFileSize(const std::string & path) {
    try {
        if (isDirectory(path))
            return calculateDirectorySize(path);
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

/**
 * 验证文件名或目录名是否合法
 * @param name 要验证的名称字符串
 * @return 如果名称合法返回true，否则返回false
 * 
 * 名称合法条件：
 * 1. 不能为空字符串
 * 2. 不能包含路径分隔符('/'或'\')
 */
bool isValidName(const std::string & name) {
    // 检查名称是否为空
    if (name.empty()) return false;
    
    // 检查是否包含路径分隔符
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
        return false;
        
    // 通过所有检查，返回合法
    return true;
}


// ---------------------------- 文件/目录操作 ----------------------------

/**
 * 创建一个新的空文件
 * @param filePath 要创建的文件的完整路径
 * @return 创建成功返回true，失败返回false
 * 
 * 函数执行流程：
 * 1. 验证文件名合法性
 * 2. 尝试创建并打开文件
 * 3. 关闭文件句柄
 * 4. 返回操作结果
 */
bool createFile(const std::string & filePath) {
    // 从路径中提取纯文件名部分
    std::string filename = fs::path(filePath).filename().string();
    
    // 检查文件名是否合法（不含非法字符）
    if (!isValidName(filename)) {
        std::cerr << "Invalid file name: " << filePath << std::endl;
        return false;
    }
    
    // 尝试创建并打开文件
    std::ofstream file(filePath);
    if (!file.is_open()) {
        // 输出详细的错误信息（包含系统错误消息）
        std::cerr << "Failed to create file " << filePath << ". Error: " 
                  << std::strerror(errno) << std::endl;
        return false;
    }
    
    // 关闭文件句柄
    file.close();
    
    // 返回操作成功
    return true;
}


/**
 * 创建新目录
 * @param dirPath 要创建的目录完整路径
 * @return 创建成功返回true，失败返回false
 * 
 * 函数执行流程：
 * 1. 验证目录名合法性
 * 2. 尝试创建目录
 * 3. 更新缓存状态
 * 4. 返回操作结果
 */
bool createDirectory(const std::string & dirPath) {
    // 从路径中提取纯目录名部分
    std::string dirname = fs::path(dirPath).filename().string();
    
    // 检查目录名是否合法（不含非法字符）
    if (!isValidName(dirname)) {
        std::cerr << "Invalid directory name: " << dirPath << std::endl;
        return false;
    }
    
    // 尝试创建目录
    if (!fs::create_directory(dirPath)) {
        // 输出详细的错误信息（包含系统错误消息）
        std::cerr << "Failed to create directory " << dirPath 
                  << ". Error: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    // 获取父目录路径用于缓存更新
    std::string parentPath = fs::path(dirPath).parent_path().string();
    {
        // 加锁保护缓存操作
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        // 如果新目录已在缓存中，则清除
        if (dir_cache.count(dirPath))
            dir_cache.erase(dirPath);
            
        // 标记父目录缓存为无效，下次访问时会重新加载
        if (dir_cache.count(parentPath))
            dir_cache[parentPath].valid = false;
    }
    
    // 返回操作成功
    return true;
}

/**
 * 删除文件或目录
 * @param path 要删除的文件或目录路径
 * @return 删除成功返回 true，失败返回 false
 * 
 * 功能说明：
 * 1. 如果是目录，递归删除目录及其所有内容
 * 2. 如果是普通文件，直接删除文件
 * 3. 捕获所有异常并返回false
 */
bool deleteFileOrDirectory(const std::string & path) {
    try {
        // 检查路径是否为目录
        if (fs::is_directory(path)) {
            // 递归删除目录及其所有内容，返回删除的条目数>0表示成功
            return fs::remove_all(path) > 0;
        }
        // 检查路径是否为普通文件
        else if (fs::is_regular_file(path)) {
            // 删除单个文件
            return fs::remove(path);
        }
    } catch (...) {
        // 捕获所有可能的异常（如权限不足、路径不存在等）
        return false;
    }
    // 如果不是目录也不是普通文件，返回失败
    return false;
}

/**
 * 进入子目录并更新相关状态
 * @param history 目录历史记录对象，用于保存浏览历史
 * @param currentPath [输入输出] 当前路径，进入后会更新为新路径
 * @param contents [输出] 新目录的内容列表
 * @param selected [输入输出] 当前选中项索引，进入后会重置为0或-1
 * 
 * 函数执行流程：
 * 1. 检查当前路径有效性
 * 2. 获取/更新目录缓存
 * 3. 验证选中项有效性
 * 4. 进入目标目录并更新缓存
 * 5. 更新历史记录和返回参数
 */
void enterDirectory(DirectoryHistory & history,
                    std::string & currentPath,
                    std::vector<std::string> & contents,
                    int & selected) {
    // 加锁保护共享资源
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // 检查当前路径是否存在且为目录
    if (!fs::exists(currentPath) || !fs::is_directory(currentPath)) {
        std::cerr << "Invalid working directory: " << currentPath << std::endl;
        selected = -1;
        return;
    }
    
    // 获取当前路径缓存，若无效则重新加载
    auto &cache = dir_cache[currentPath];
    if (!cache.valid) {
        cache.contents = getDirectoryContents(currentPath);
        cache.valid = true;
        cache.last_update = std::chrono::system_clock::now();
    }
    
    // 验证选中项是否在有效范围内
    bool valid_selection = !cache.contents.empty() && (selected >= 0) &&
                         (static_cast<size_t>(selected) < cache.contents.size());
    if (!valid_selection) {
        selected = -1;
        return;
    }
    
    // 构建目标目录完整路径
    fs::path fullPath = fs::path(currentPath) / cache.contents[selected];
    if (!fs::exists(fullPath) || !isDirectory(fullPath.string())) {
        std::cerr << "Target is not a valid directory: " << fullPath << std::endl;
        selected = -1;
        return;
    }
    
    // 预加载目标目录内容到缓存
    auto &new_cache = dir_cache[fullPath.string()];
    new_cache.contents = getDirectoryContents(fullPath.string());
    new_cache.valid = true;
    new_cache.last_update = std::chrono::system_clock::now();
    
    // 更新历史记录和当前路径
    history.push(currentPath);
    currentPath = fullPath.lexically_normal().string();
    
    // 设置返回参数
    contents = new_cache.contents;
    selected = contents.empty() ? -1 : 0;  // 空目录设为-1，否则选中第一项
}

/**
 * 统计目录中的文件和子目录数量，并收集权限信息
 * @param path 要统计的目标目录路径
 * @param file_count [输出] 文件数量统计结果
 * @param folder_count [输出] 子目录数量统计结果
 * @param folder_permissions [输出] 子目录权限信息，存储为(名称, 权限位)元组
 * @param fileNames [输出] 目录中所有条目名称列表
 * 
 * 功能说明：
 * 1. 统计目录中的文件和子目录数量
 * 2. 收集子目录的权限信息
 * 3. 记录所有条目名称
 * 4. 异常安全处理
 */
void calculation_current_folder_files_number(
    const std::string & path,
    int & file_count,
    int & folder_count,
    std::vector<std::tuple<std::string, mode_t>> & folder_permissions,
    std::vector<std::string> & fileNames) {
    // 初始化输出参数
    file_count = 0;
    folder_count = 0;
    folder_permissions.clear();
    fileNames.clear();
    
    try {
        // 遍历目录中的所有条目
        for (const auto &entry : fs::directory_iterator(path)) {
            // 获取当前条目名称
            std::string name = entry.path().filename().string();
            fileNames.push_back(name); // 记录所有条目名称
            
            if (entry.is_directory()) {
                folder_count++; // 统计子目录数量
                
                // 获取子目录权限信息
                struct stat fileStat;
                if (stat(entry.path().c_str(), &fileStat) == 0) {
                    // 保存(名称, 权限位)对
                    folder_permissions.emplace_back(name, fileStat.st_mode);
                } else {
                    std::cerr << "Error getting permissions for " << entry.path() << std::endl;
                }
            } 
            // 统计普通文件数量
            else if (entry.is_regular_file()) {
                file_count++; 
            }
        }
    } catch (const std::exception &e) {
        // 捕获并记录遍历异常
        std::cerr << "Error counting entries in " << path << ": " << e.what() << std::endl;
    }
}

// ---------------------------- 文件内容读写与缓存管理 ----------------------------

/**
 * 读取文件指定行范围内的内容，支持简单的缓存机制
 * @param filePath 要读取的文件路径
 * @param startLine 起始行号（从 1 开始）
 * @param endLine 结束行号（包含在内）
 * @return 返回由指定行拼接而成的字符串，若出错则返回错误信息字符串
 */
std::string readFileContent(const std::string & filePath, size_t startLine, size_t endLine) {
    const std::chrono::seconds expiry(60); // 缓存有效期：60 秒
    std::lock_guard<std::mutex> lock(cache_mutex);
    // 检查文件内容缓存
    auto it = fileChunkCache.find(filePath);
    if (it != fileChunkCache.end()) {
        auto now = std::chrono::system_clock::now();
        // 如果缓存未过期，则可以直接复用（调试信息打印）
        if (now - it->second.last_update < expiry) {
            std::cerr << "[DEBUG] Using cached content for: " << filePath << std::endl;
        } else {
            fileChunkCache.erase(it); // 否则清除过期缓存
        }
    }
    // 打开文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filePath << std::endl;
        return "错误: 无法打开文件";
    }
    // 按行读取，拼接指定行范围的内容
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
    // 更新缓存：这里只缓存最后一次读取的行范围内容，键为文件路径
    fileChunkCache[filePath] = FileChunkCache{{ {0, result} }, std::chrono::system_clock::now()};
    return result;
}

/**
 * 写入内容到指定文件并更新缓存状态
 * @param filePath 要写入的文件路径
 * @param content 要写入的内容字符串
 * @return 写入成功返回true，失败返回false
 * 
 * 功能说明：
 * 1. 以截断模式打开文件并写入内容
 * 2. 更新相关缓存状态
 * 3. 线程安全操作
 */
bool writeFileContent(const std::string & filePath, const std::string & content) {
    // 以截断模式打开文件
    std::ofstream ofs(filePath, std::ios::trunc);
    if (!ofs) {
        std::cerr << "[ERROR] Cannot open file for writing: " << filePath << std::endl;
        return false;
    }
    
    // 写入内容并关闭文件
    ofs << content;
    ofs.close();
    
    {
        // 加锁保护缓存操作
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        // 清除该文件的缓存内容
        if (fileChunkCache.count(filePath)) {
            fileChunkCache.erase(filePath);
        }
        
        // 获取父目录路径并标记其缓存为无效
        std::string parentDir = fs::path(filePath).parent_path().string();
        if (dir_cache.count(parentDir)) {
            dir_cache[parentDir].valid = false;
        }
    }
    
    return true;
}


/**
 * 清理过期的文件内容缓存
 * @param expiry 缓存有效期阈值，超过此时长的缓存将被清除
 * 
 * 功能说明：
 * 1. 遍历所有文件内容缓存
 * 2. 删除超过有效期的缓存项
 * 3. 线程安全操作
 */
void clearFileChunkCache(const std::chrono::seconds& expiry) {
    // 加锁保护缓存操作
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    
    // 遍历缓存字典
    for (auto it = fileChunkCache.begin(); it != fileChunkCache.end(); ) {
        // 检查缓存项是否过期
        if (now - it->second.last_update > expiry) {
            // 删除过期缓存项
            it = fileChunkCache.erase(it);
        } else {
            // 保留未过期缓存项
            ++it;
        }
    }
}


// ---------------------------- 文件/目录重命名 ----------------------------

/**
 * 重命名文件或目录并更新缓存
 * @param oldPath 原文件/目录路径
 * @param newName 新名称(不含路径)
 * @return 操作成功返回true，失败返回false
 * 
 * 功能说明：
 * 1. 验证新名称合法性
 * 2. 检查目标路径是否存在
 * 3. 执行重命名操作
 * 4. 更新相关缓存
 * 5. 异常安全处理
 */
bool renameFileOrDirectory(const std::string& oldPath, const std::string& newName) {
    // 验证新名称是否合法(不含非法字符)
    if (!isValidName(newName)) {
        std::cerr << "❌ 无效的名称: " << newName << std::endl;
        return false;
    }
    
    // 构造新旧完整路径
    fs::path oldFilePath(oldPath);
    fs::path newFilePath = oldFilePath.parent_path() / newName;
    
    try {
        // 检查目标路径是否已存在
        if (fs::exists(newFilePath)) {
            std::cerr << "❌ 目标文件/文件夹已存在: " << newFilePath << std::endl;
            return false;
        }
        
        // 执行实际重命名操作
        fs::rename(oldPath, newFilePath);
        
        // 获取父目录路径用于缓存更新
        std::string parentPath = oldFilePath.parent_path().string();
        {
            // 加锁保护缓存操作
            std::lock_guard<std::mutex> lock(cache_mutex);
            
            // 标记父目录缓存为无效(下次访问会重新加载)
            if (dir_cache.count(parentPath)) {
                dir_cache[parentPath].valid = false;
            }
            
            // 清除旧路径的缓存(如果存在)
            if (dir_cache.count(oldPath)) {
                dir_cache.erase(oldPath);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        // 捕获并记录重命名过程中的异常
        std::cerr << "❌ 重命名失败: " << e.what() << std::endl;
        return false;
    }
}


} // namespace FileManager
