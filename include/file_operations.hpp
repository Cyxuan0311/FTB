#pragma once
#include <stack>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include "../include/file_browser.hpp"

// 定义DirectoryCache结构体
struct DirectoryCache {
    std::vector<std::string> contents;
    std::chrono::system_clock::time_point last_update;
    bool valid = false;
};

namespace fs = std::filesystem;

extern std::mutex cache_mutex;
extern std::unordered_map<std::string, DirectoryCache> dir_cache;


void enterDirectory(std::stack<std::string>& pathHistory, 
                   std::string& currentPath,
                   std::vector<std::string>& contents,
                   int& selected);