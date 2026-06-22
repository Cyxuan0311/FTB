#ifndef FILE_SIZE_CALCULATOR_HPP
#define FILE_SIZE_CALCULATOR_HPP

#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace FileSizeCalculator {

void CalculateSizes(const std::string& path,
                    int selected,
                    std::atomic<uintmax_t>& total_folder_size,
                    std::atomic<double>& size_ratio,
                    std::string& selected_size);

// 异步版本：在后台线程计算，不阻塞 UI
void CalculateSizesAsync(const std::string& path,
                         int selected,
                         std::atomic<uintmax_t>& total_folder_size,
                         std::atomic<double>& size_ratio,
                         std::string& selected_size);

// 检查异步计算是否完成
bool IsCalculating();

// 请求取消当前计算
void RequestCancel();

}

#endif // FILE_SIZE_CALCULATOR_HPP
