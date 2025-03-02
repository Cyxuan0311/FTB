#include "../include/file_browser.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <filesystem>

namespace FileBrowser {
    bool isDirectory(const std::string& path) {
        struct stat statbuf;
        return stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
    }

    std::vector<std::string> getDirectoryContents(const std::string& path) {
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
    std::string formatTime(const std::tm& time) {
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
        return buffer;
    }

    uintmax_t getFileSize(const std::string& path) {
        uintmax_t total_size=0;
        try {
            if (isDirectory(path)) {
                return calculateDirectorySize(path);
            }
            return std::filesystem::file_size(path);
        } catch (...) {
            return 0;
        }
        return total_size;
    }
    
    uintmax_t calculateDirectorySize(const std::string& path) {
        uintmax_t total = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (!std::filesystem::is_directory(entry)) {
                    total += std::filesystem::file_size(entry);
                }
            }
        } catch (...) {
            return 0;
        }
        return total;
    }
}