#include "FileManager.hpp"
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <numeric>

namespace fs = std::filesystem;

namespace FileManager {

    std::mutex cache_mutex;
    std::unordered_map<std::string, DirectoryCache> dir_cache;

    bool isDirectory(const std::string & path) {
        struct stat statbuf;
        return stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
    }

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

    std::string formatTime(const std::tm & time) {
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
        return buffer;
    }

    uintmax_t calculateDirectorySize(const std::string & path) {
        uintmax_t total = 0;
        try {
            for (const auto & entry : fs::recursive_directory_iterator(path)) {
                if (!fs::is_directory(entry)) {
                    total += fs::file_size(entry);
                }
            }
        } catch (...) {
            return 0;
        }
        return total;
    }

    uintmax_t getFileSize(const std::string & path) {
        try {
            if (isDirectory(path)) {
                return calculateDirectorySize(path);
            }
            return fs::file_size(path);
        } catch (...) {
            return 0;
        }
    }

    bool isValidName(const std::string & name) {
        if (name.empty()) return false;
        // 简单检查：不允许包含斜杠
        if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
            return false;
        return true;
    }

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
            if (dir_cache.count(dirPath)) {
                dir_cache.erase(dirPath);
            }
            if (dir_cache.count(parentPath)) {
                dir_cache[parentPath].valid = false;
            }
        }
        return true;
    }

    bool deleteFileOrDirectory(const std::string & path) {
        if (fs::is_directory(path)) {
            return fs::remove_all(path);
        } else if (fs::is_regular_file(path)) {
            return fs::remove(path);
        }
        return false;
    }

    void enterDirectory(std::stack<std::string>& pathHistory,
                        std::string& currentPath,
                        std::vector<std::string>& contents,
                        int& selected) {
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
        pathHistory.push(currentPath);
        currentPath = fullPath.lexically_normal().string();
        contents = new_cache.contents;
        selected = contents.empty() ? -1 : 0;
    }

    void calculation_current_folder_files_number(const std::string & path, int &file_count, int &folder_count) {
        file_count = 0;
        folder_count = 0;
        try {
            for (const auto & entry : fs::directory_iterator(path)) {
                if (entry.is_directory()) {
                    folder_count++;
                } else if (entry.is_regular_file()) {
                    file_count++;
                }
            }
        } catch (const std::exception & e) {
            std::cerr << "Error counting entries in " << path << ": " << e.what() << std::endl;
        }
    }
    
}
