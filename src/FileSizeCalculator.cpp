#include "../include/FTB/FileSizeCalculator.hpp"
#include "../include/FTB/FileManager.hpp"  // 假设 FileManager 定义了 dir_cache 和 cache_mutex
#include <numeric>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace FileSizeCalculator {
// 计算文件夹大小和选择项的大小
void CalculateSizes(const std::string& path,
                    int selected,
                    std::atomic<uintmax_t>& total_folder_size,
                    std::atomic<double>& size_ratio,
                    std::string& selected_size) {
    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
    auto& cache = FileManager::dir_cache[path];

    if (!cache.valid) {
        cache.contents = FileManager::getDirectoryContents(path);
        cache.last_update = std::chrono::system_clock::now();
        cache.valid = true;
        cache.sizes.clear();
        cache.total_size = 0;
    }

    if (cache.contents.empty()) {
        total_folder_size.store(0, std::memory_order_relaxed);
        size_ratio.store(0.0, std::memory_order_relaxed);
        selected_size = "0 B";
        return;
    }

    if (cache.sizes.empty()) {
        for (const auto& item : cache.contents) {
            std::string fullPath = (fs::path(path) / item).string();
            uintmax_t fsize = FileManager::getFileSize(fullPath);
            cache.sizes.push_back(fsize);
        }
        cache.total_size = std::accumulate(cache.sizes.begin(), cache.sizes.end(), 0ULL);
    }

    total_folder_size.store(cache.total_size, std::memory_order_relaxed);

    if (selected >= 0 && selected < static_cast<int>(cache.sizes.size())) {
        uintmax_t size = cache.sizes[selected];
        double ratio = cache.total_size > 0 ? static_cast<double>(size) / cache.total_size : 0.0;
        size_ratio.store(ratio, std::memory_order_relaxed);

        std::ostringstream oss;
        if (size >= 1024 * 1024) {
            oss << std::fixed << std::setprecision(2)
                << (size / (1024.0 * 1024.0)) << " MB";
        } else if (size >= 1024) {
            oss << std::fixed << std::setprecision(2)
                << (size / 1024.0) << " KB";
        } else {
            oss << size << " B";
        }
        selected_size = oss.str();
    } else {
        selected_size = "0 B";
    }
}

} // namespace FileSizeCalculator
