// FileSizeCalculator.cpp - 文件大小计算器实现文件
#include "../include/FTB/FileSizeCalculator.hpp"

#include <execution>   // 并行算法支持
#include <filesystem>  // 文件系统操作
#include <iomanip>     // 格式化输出
#include <mutex>       // 互斥锁
#include <numeric>     // 数值算法（accumulate）
#include <sstream>     // 字符串流，用于构造输出字符串
#include <vector>      // 向量容器

#include "../include/FTB/FileManager.hpp"

namespace fs = std::filesystem;  // 文件系统命名空间别名

namespace FileSizeCalculator
{

    // --------------------------------------------------
    // 计算指定目录下所有文件的大小，并更新选中文件的大小比例与可读字符串
    // 参数:
    //   path                - 要计算的目录路径
    //   selected            - 当前选中文件在目录列表中的索引
    //   total_folder_size   - 输出参数，存储目录总大小（原子变量）
    //   size_ratio          - 输出参数，存储选中文件占总大小的比例（原子变量）
    //   selected_size       - 输出参数，存储选中文件的人类可读大小字符串
    // 说明:
    //   - 首先尝试从 FileManager 的缓存中获取目录内容列表和各文件大小
    //   - 如果缓存无效或大小数据为空，则并行计算每个文件的大小，并存入缓存
    //   - 计算完总大小后，根据 selected 索引计算该文件的大小占比，并格式化为 KB/MB/B 单位
    // --------------------------------------------------
    void CalculateSizes(const std::string& path, int selected,
                        std::atomic<uintmax_t>& total_folder_size,
                        std::atomic<double>& size_ratio,
                        std::string& selected_size)
    {
        // -------------------- 获取目录内容副本 --------------------
        std::vector<std::string> contents_copy;
        {
            // 使用互斥锁保护缓存访问
            std::unique_lock<std::mutex> lock(FileManager::cache_mutex);
            auto& cache = FileManager::dir_cache[path];

            // 如果缓存无效，则重新获取目录内容列表
            if (!cache.valid || !cache.is_still_valid(path))
            {
                lock.unlock(); // 释放锁
                std::unique_lock<std::mutex> write_lock(FileManager::cache_mutex);
                
                // 双重检查
                if (!cache.valid || !cache.is_still_valid(path)) {
                    cache.contents    = FileManager::getDirectoryContents(path);
                    cache.last_update = std::chrono::system_clock::now();
                    cache.valid       = true;
                    cache.sizes.clear();
                    cache.total_size = 0;
                    cache.file_mod_times.clear();
                }
                write_lock.unlock();
                lock.lock(); // 重新获取锁
            }
            // 拷贝目录内容以供后续计算使用
            contents_copy = cache.contents;
        }

        // 如果目录为空，则直接将所有输出设置为 0 或空，并返回
        if (contents_copy.empty())
        {
            total_folder_size.store(0, std::memory_order_relaxed);
            size_ratio.store(0.0, std::memory_order_relaxed);
            selected_size = "0 B";
            return;
        }

        // -------------------- 尝试获取文件大小缓存 --------------------
        std::vector<uintmax_t> sizes;
        {
            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
            auto& cache = FileManager::dir_cache[path];
            // 如果缓存中已经有大小数据，则直接复制
            if (!cache.sizes.empty())
            {
                sizes = cache.sizes;
            }
        }

        // -------------------- 并行计算文件大小（若缓存为空） --------------------
        if (sizes.empty())
        {
            // 调整 sizes 大小以存放每个文件的大小
            sizes.resize(contents_copy.size());
            // 使用并行算法 transform 遍历每个文件名，计算其大小
            std::transform(std::execution::par,
                           contents_copy.begin(), contents_copy.end(),
                           sizes.begin(),
                           [&](const std::string& item) {
                               // 拼接完整路径并调用 FileManager 获取文件大小
                               std::string fullPath = (fs::path(path) / item).string();
                               return FileManager::getFileSize(fullPath);
                           });

            // 并行计算得到大小数组后，累加获取目录总大小
            uintmax_t total = std::accumulate(sizes.begin(), sizes.end(), 0ULL);

            // 更新缓存：存储各文件大小和总大小
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                auto& cache = FileManager::dir_cache[path];
                cache.sizes      = sizes;
                cache.total_size = total;
            }
        }

        // -------------------- 获取目录总大小 --------------------
        uintmax_t totalSize = 0;
        {
            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
            totalSize = FileManager::dir_cache[path].total_size;
        }

        // 将目录总大小存入原子变量以便 UI 线程安全访问
        total_folder_size.store(totalSize, std::memory_order_relaxed);

        // -------------------- 根据 selected 计算选中文件大小与比例 --------------------
        if (selected >= 0 && static_cast<size_t>(selected) < sizes.size())
        {
            uintmax_t size  = sizes[selected];
            double    ratio = (totalSize > 0) ? (static_cast<double>(size) / totalSize) : 0.0;
            size_ratio.store(ratio, std::memory_order_relaxed);

            // 将选中文件大小格式化为可读字符串（用 B, KB, MB 单位）
            std::ostringstream oss;
            if (size >= 1024 * 1024)
            {
                // 大于等于 1 MB，显示 MB 单位
                oss << std::fixed << std::setprecision(2)
                    << (size / (1024.0 * 1024.0)) << " MB";
            }
            else if (size >= 1024)
            {
                // 大于等于 1 KB，显示 KB 单位
                oss << std::fixed << std::setprecision(2)
                    << (size / 1024.0) << " KB";
            }
            else
            {
                // 小于 1 KB，显示字节数
                oss << size << " B";
            }
            selected_size = oss.str();
        }
        else
        {
            // selected 索引无效或未选中任何文件，则输出 0 B
            selected_size = "0 B";
        }
    }

}  // namespace FileSizeCalculator
