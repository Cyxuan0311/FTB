#ifndef FILE_SIZE_CALCULATOR_HPP
#define FILE_SIZE_CALCULATOR_HPP

#include <string>
#include <atomic>

namespace FileSizeCalculator {

void CalculateSizes(const std::string& path,
                    int selected,
                    std::atomic<uintmax_t>& total_folder_size,
                    std::atomic<double>& size_ratio,
                    std::string& selected_size);

}

#endif // FILE_SIZE_CALCULATOR_HPP
