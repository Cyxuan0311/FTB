#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <sys/stat.h>    // 用于 stat 函数获取文件/目录元信息（例如权限、大小等）

#include <chrono>        // 用于时间点类型（缓存更新时间等）
#include <map>           // 用于存储缓存数据映射
#include <mutex>         // 用于线程间互斥锁，保证缓存访问线程安全
#include <string>        // std::string 类型
#include <tuple>         // std::tuple，用于打包文件夹名与权限信息
#include <vector>        // std::vector，用于存储目录内容列表
#include <shared_mutex>  // 读写锁，提高并发性能
#include <atomic>        // 原子操作
#include <thread>        // 线程支持

#include "DirectoryHistory.hpp"  // 目录历史记录，用于记录进入/返回操作
#include "LRUCache.hpp"          // LRU缓存实现

// 定义读取文件时的分块大小：8KB，用于分块加载大文件内容
constexpr size_t CHUNK_SIZE = 8192;  // 8KB

namespace FileManager
{

    /**
     * @struct DirectoryCache
     * @brief 用于缓存某个目录的内容信息，避免频繁磁盘遍历带来的性能开销
     *
     * 成员变量：
     *   valid        - 缓存是否有效，若为 false 则需要重新读取目录内容
     *   contents     - 该目录下所有子文件/子目录的名称列表
     *   sizes        - 与 contents 对应的每个条目的大小（filesize 或子目录大小）
     *   total_size   - 整个目录的总大小（递归计算）
     *   last_update  - 本次缓存更新时间戳，用于判断是否过期
     *   file_mod_times - 文件修改时间，用于智能缓存失效
     */
    struct DirectoryCache
    {
        bool                                  valid = false;
        std::vector<std::string>              contents;
        std::vector<uintmax_t>                sizes;
        uintmax_t                             total_size = 0;
        std::chrono::system_clock::time_point last_update;
        std::vector<std::chrono::system_clock::time_point> file_mod_times;
        
        DirectoryCache() = default;
        
        // 检查缓存是否仍然有效（基于文件修改时间）
        bool is_still_valid(const std::string& path) const {
            if (!valid) return false;
            
            try {
                auto current_time = std::filesystem::last_write_time(path);
                auto cache_time = std::chrono::system_clock::from_time_t(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        current_time.time_since_epoch()).count());
                
                return cache_time <= last_update;
            } catch (...) {
                return false;
            }
        }
    };

    /**
     * @struct FileChunkCache
     * @brief 用于缓存文件分块读取后的内容，支持分块加载大文件
     *
     * 成员变量：
     *   chunks       - 键为块索引（size_t），值为对应块的字符串内容
     *   last_update  - 本次缓存更新时间戳，用于判断是否过期
     *   file_size    - 文件总大小，用于验证缓存有效性
     */
    struct FileChunkCache
    {
        std::map<size_t, std::string>         chunks;
        std::chrono::system_clock::time_point last_update;
        uintmax_t                             file_size = 0;
        
        FileChunkCache() = default;
        
        // 检查缓存是否仍然有效
        bool is_still_valid(const std::string& file_path) const {
            try {
                if (std::filesystem::file_size(file_path) != file_size) {
                    return false;
                }
                
                auto current_time = std::filesystem::last_write_time(file_path);
                auto cache_time = std::chrono::system_clock::from_time_t(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        current_time.time_since_epoch()).count());
                
                return cache_time <= last_update;
            } catch (...) {
                return false;
            }
        }
    };

    // ---------------------------- 全局缓存变量声明 ----------------------------

    /// 保护缓存访问的读写锁，提高并发性能
    extern std::shared_mutex                     cache_mutex;
    /// 目录路径到 DirectoryCache 的映射，用于缓存各个目录的内容信息
    extern std::map<std::string, DirectoryCache> dir_cache;
    /// 文件路径到 FileChunkCache 的映射，用于缓存文件分块读取的数据
    extern std::map<std::string, FileChunkCache> fileChunkCache;
    
    /// 优化的LRU缓存，用于目录内容缓存
    extern LRUCache<std::string, DirectoryCache> lru_dir_cache;
    /// 优化的LRU缓存，用于文件大小缓存
    extern LRUCache<std::string, uintmax_t>      lru_size_cache;
    /// 优化的LRU缓存，用于文件内容缓存
    extern LRUCache<std::string, std::string>    lru_content_cache;
    
    /// 缓存统计信息
    extern std::atomic<size_t>                   cache_hits;
    extern std::atomic<size_t>                   cache_misses;
    extern std::atomic<size_t>                   cache_evictions;

    // ---------------------------- 接口声明 ----------------------------

    /**
     * @brief 判断给定路径是否为目录
     * @param path 要检查的路径
     * @return 如果是目录返回 true，否则返回 false（包含路径不存在的情况）
     */
    bool                     isDirectory(const std::string& path);

    /**
     * @brief 获取指定目录下的文件和子目录名称列表（不含 "." 和 ".."）
     * @param path 目录路径
     * @return 返回名称列表（不包含隐藏 "."、".." 条目）
     */
    std::vector<std::string> getDirectoryContents(const std::string& path);

    /**
     * @brief 将 std::tm 结构化时间格式化为 "YYYY-MM-DD HH:MM:SS" 字符串
     * @param time 要格式化的时间结构体
     * @return 返回格式化后的人类可读时间字符串
     */
    std::string              formatTime(const std::tm& time);

    /**
     * @brief 递归计算目录大小（包含所有子目录和文件）
     * @param path 要计算大小的目录路径
     * @return 返回该目录及其子目录所有文件总大小（字节数），如果出错返回 0
     */
    uintmax_t                calculateDirectorySize(const std::string& path);

    /**
     * @brief 获取文件或目录的大小
     * @param path 文件或目录路径
     * @return 如果是目录，则调用 calculateDirectorySize；如果是文件，则调用 fs::file_size；错误时返回 0
     */
    uintmax_t                getFileSize(const std::string& path);

    /**
     * @brief 验证提供的名称是否合法（不允许空、也不允许包含 '/' 或 '\'）
     * @param name 文件/目录名称（不含路径）
     * @return 合法返回 true，否则 false
     */
    bool                     isValidName(const std::string& name);

    /**
     * @brief 在磁盘上创建一个空文件
     * @param filePath 目标文件的完整路径（含文件名）
     * @return 创建成功返回 true，否则 false
     */
    bool                     createFile(const std::string& filePath);

    /**
     * @brief 在磁盘上创建一个目录
     * @param dirPath 目标目录的完整路径
     * @return 创建成功返回 true，否则 false
     */
    bool                     createDirectory(const std::string& dirPath);

    /**
     * @brief 删除指定路径的文件或目录（如果是目录则递归删除所有子项）
     * @param path 要删除的文件或目录路径
     * @return 删除成功返回 true，否则 false
     */
    bool                     deleteFileOrDirectory(const std::string& path);

    /**
     * @brief 进入子目录，并利用缓存机制加速读取
     * @param history   目录历史记录，用于后退操作
     * @param currentPath [输入/输出] 当前工作目录，进入成功后更新为新子目录路径
     * @param contents  [输出] 新目录下的内容列表
     * @param selected  [输入/输出] 原先选中的下标；进入新目录后更新为新目录内容中第一个合法下标，否则为 -1
     */
    void enterDirectory(DirectoryHistory& history,
                        std::string& currentPath,
                        std::vector<std::string>& contents,
                        int& selected);

    /**
     * @brief 计算当前目录下的文件数、子目录数，并获取子目录的权限信息，同时收集所有条目名称
     * @param path               目标目录路径
     * @param file_count         [输出] 该目录下文件数量
     * @param folder_count       [输出] 该目录下子目录数量
     * @param folder_permissions [输出] 每个子目录的 (名称, mode_t 权限位) 信息
     * @param fileNames          [输出] 目录下所有条目（文件+目录）的名称列表
     */
    void calculation_current_folder_files_number(
        const std::string& path,
        int& file_count,
        int& folder_count,
        std::vector<std::tuple<std::string, mode_t>>& folder_permissions,
        std::vector<std::string>& fileNames);

    /**
     * @brief 读取文件指定行范围的内容，并使用分块缓存优化大文件读取
     * @param filePath 文件路径
     * @param startLine 起始行号（从 1 开始）
     * @param endLine   结束行号（包含此行）
     * @return 返回拼接后的文本内容，若无法打开文件则返回错误信息字符串
     */
    std::string readFileContent(const std::string& filePath,
                                size_t startLine,
                                size_t endLine);

    /**
     * @brief 将指定内容写入到文件，并清除与该文件相关的缓存
     * @param filePath 文件路径
     * @param content  要写入的完整内容
     * @return 写入成功返回 true，否则 false
     */
    bool        writeFileContent(const std::string& filePath,
                                 const std::string& content);

    /**
     * @brief 清理已过期的文件块缓存
     * @param expiry 缓存有效期阈值，若缓存更新时间距今超过此值，则清除该缓存
     */
    void clearFileChunkCache(const std::chrono::seconds& expiry);

    /**
     * @brief 重命名文件或目录，并更新缓存状态
     * @param oldPath 原文件/目录路径
     * @param newName 新名称（仅名称部分，不含路径）
     * @return 重命名成功返回 true，否则 false
     */
    bool renameFileOrDirectory(const std::string& oldPath,
                               const std::string& newName);

    // ---------------------------- 缓存管理接口 ----------------------------
    
    /**
     * @brief 初始化缓存系统
     * @param max_dir_cache_size 目录缓存最大大小
     * @param max_size_cache_size 大小缓存最大大小
     * @param max_content_cache_size 内容缓存最大大小
     * @param enable_persistence 是否启用持久化
     */
    void initializeCacheSystem(size_t max_dir_cache_size = 1000,
                              size_t max_size_cache_size = 5000,
                              size_t max_content_cache_size = 2000,
                              bool enable_persistence = true);
    
    /**
     * @brief 清理所有缓存
     */
    void clearAllCaches();
    
    /**
     * @brief 清理过期缓存
     * @return 清理的缓存项数量
     */
    size_t cleanupExpiredCaches();
    
    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStatistics {
        size_t dir_cache_size;
        size_t size_cache_size;
        size_t content_cache_size;
        size_t total_hits;
        size_t total_misses;
        size_t total_evictions;
        double hit_ratio;
    };
    
    CacheStatistics getCacheStatistics();
    
    /**
     * @brief 预热缓存（预加载常用目录）
     * @param paths 要预热的目录路径列表
     */
    void warmupCache(const std::vector<std::string>& paths);
    
    /**
     * @brief 智能缓存失效（基于文件系统事件）
     * @param path 发生变化的路径
     */
    void invalidateCacheForPath(const std::string& path);

}  // namespace FileManager

#endif  // FILE_MANAGER_HPP
