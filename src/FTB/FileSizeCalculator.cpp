// FileSizeCalculator.cpp - 文件大小计算器实现文件
#include "../include/FTB/FileSizeCalculator.hpp"

#include <filesystem>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <sstream>
#include <vector>

#include "../include/FTB/FileManager.hpp"

namespace fs = std::filesystem;

namespace FileSizeCalculator
{

    // ---- 异步计算状态 ----
    static std::atomic<bool> g_calculating{false};
    static std::atomic<bool> g_cancel_requested{false};
    static std::mutex g_calc_mutex;
    static std::thread g_calc_thread;

    bool IsCalculating() { return g_calculating.load(); }
    void RequestCancel() { g_cancel_requested.store(true); }

    // ---- 格式化文件大小 ----
    static std::string FormatSize(uintmax_t size) {
        std::ostringstream oss;
        if (size >= 1024 * 1024)
            oss << std::fixed << std::setprecision(2) << (size / (1024.0 * 1024.0)) << " MB";
        else if (size >= 1024)
            oss << std::fixed << std::setprecision(2) << (size / 1024.0) << " KB";
        else
            oss << size << " B";
        return oss.str();
    }

    // ---- 同步计算 (保留原接口) ----
    void CalculateSizes(const std::string& path, int selected,
                        std::atomic<uintmax_t>& total_folder_size,
                        std::atomic<double>& size_ratio,
                        std::string& selected_size)
    {

        std::vector<std::string> contents_copy;
        {
            std::unique_lock<std::mutex> lock(FileManager::cache_mutex);
            auto cached_result = FileManager::lru_dir_cache->get(path);
            if (cached_result.has_value()) {
                contents_copy = cached_result->contents;
            } else {
                lock.unlock();
                contents_copy = FileManager::getDirectoryContents(path);
                lock.lock();
            }
        }

        if (contents_copy.empty()) {
            total_folder_size.store(0, std::memory_order_relaxed);
            size_ratio.store(0.0, std::memory_order_relaxed);
            selected_size = "0 B";
            return;
        }

        // 尝试从大小缓存获取
        uintmax_t totalSize = 0;
        std::vector<uintmax_t> sizes;
        {
            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
            auto cached_size = FileManager::lru_size_cache->get(path);
            if (cached_size.has_value()) {
                totalSize = cached_size.value();
            }
        }

        if (totalSize == 0) {
            sizes.resize(contents_copy.size());
            std::transform(
                           contents_copy.begin(), contents_copy.end(),
                           sizes.begin(),
                           [&](const std::string& item) {
                               std::string fullPath = (fs::path(path) / item).string();
                               return FileManager::getFileSize(fullPath);
                           });
            totalSize = std::accumulate(sizes.begin(), sizes.end(), uintmax_t(0));
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_size_cache->put(path, totalSize);
            }
        }

        total_folder_size.store(totalSize, std::memory_order_relaxed);

        if (selected >= 0 && static_cast<size_t>(selected) < contents_copy.size()) {
            // 如果 sizes 为空（从缓存获取的 totalSize），单独计算选中文件大小
            uintmax_t sel_size = 0;
            if (!sizes.empty()) {
                sel_size = sizes[selected];
            } else {
                std::string fullPath = (fs::path(path) / contents_copy[selected]).string();
                sel_size = FileManager::getFileSize(fullPath);
            }
            double ratio = (totalSize > 0) ? (static_cast<double>(sel_size) / totalSize) : 0.0;
            size_ratio.store(ratio, std::memory_order_relaxed);
            selected_size = FormatSize(sel_size);
        } else {
            selected_size = "0 B";
        }
    }

    // ---- 异步计算 ----
    void CalculateSizesAsync(const std::string& path, int selected,
                             std::atomic<uintmax_t>& total_folder_size,
                             std::atomic<double>& size_ratio,
                             std::string& selected_size)
    {
        // 如果已在计算，请求取消并等待
        if (g_calculating.load()) {
            g_cancel_requested.store(true);
            if (g_calc_thread.joinable())
                g_calc_thread.join();
        }

        g_calculating.store(true);
        g_cancel_requested.store(false);

        g_calc_thread = std::thread([&path, selected,
                                       &total_folder_size, &size_ratio, &selected_size]() {
            std::vector<std::string> contents_copy;
            {
                std::unique_lock<std::mutex> lock(FileManager::cache_mutex);
                auto cached_result = FileManager::lru_dir_cache->get(path);
                if (cached_result.has_value()) {
                    contents_copy = cached_result->contents;
                } else {
                    lock.unlock();
                    contents_copy = FileManager::getDirectoryContents(path);
                    lock.lock();
                }
            }

            if (g_cancel_requested.load()) {
                g_calculating.store(false);
                return;
            }

            if (contents_copy.empty()) {
                total_folder_size.store(0, std::memory_order_relaxed);
                size_ratio.store(0.0, std::memory_order_relaxed);
                selected_size = "0 B";
                g_calculating.store(false);
                return;
            }

            // 尝试从大小缓存获取
            uintmax_t totalSize = 0;
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                auto cached_size = FileManager::lru_size_cache->get(path);
                if (cached_size.has_value()) {
                    totalSize = cached_size.value();
                }
            }

            std::vector<uintmax_t> sizes;
            if (totalSize == 0) {
                sizes.resize(contents_copy.size());
                std::transform(
                               contents_copy.begin(), contents_copy.end(),
                               sizes.begin(),
                               [&](const std::string& item) {
                                   if (g_cancel_requested.load()) return uintmax_t(0);
                                   std::string fullPath = (fs::path(path) / item).string();
                                   return FileManager::getFileSize(fullPath);
                               });

                if (g_cancel_requested.load()) {
                    g_calculating.store(false);
                    return;
                }

                totalSize = std::accumulate(sizes.begin(), sizes.end(), uintmax_t(0));
                {
                    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                    FileManager::lru_size_cache->put(path, totalSize);
                }
            }

            total_folder_size.store(totalSize, std::memory_order_relaxed);

            if (selected >= 0 && static_cast<size_t>(selected) < contents_copy.size()) {
                uintmax_t sel_size = 0;
                if (!sizes.empty()) {
                    sel_size = sizes[selected];
                } else {
                    std::string fullPath = (fs::path(path) / contents_copy[selected]).string();
                    sel_size = FileManager::getFileSize(fullPath);
                }
                double ratio = (totalSize > 0) ? (static_cast<double>(sel_size) / totalSize) : 0.0;
                size_ratio.store(ratio, std::memory_order_relaxed);
                selected_size = FormatSize(sel_size);
            } else {
                selected_size = "0 B";
            }

            g_calculating.store(false);
        });

        g_calc_thread.detach();
    }

}  // namespace FileSizeCalculator
