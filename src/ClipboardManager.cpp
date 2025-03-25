#include "../include/FTB/ClipboardManager.hpp"
#include <algorithm>
#include <stdexcept>

void ClipboardManager::addItem(const std::string& path) {
    if (std::find(items.begin(), items.end(), path) == items.end()) {
        items.push_back(path);
    }
}

bool ClipboardManager::paste(const std::string& targetPath) {
    if (items.empty()) return false;
    
    for (const auto& sourcePath : items) {
        fs::path src(sourcePath);
        fs::path dst = fs::path(targetPath) / src.filename();
        
        try {
            if (cutMode) {
                fs::rename(src, dst);
            } else {
                if (fs::is_directory(src)) {
                    fs::copy(src, dst, fs::copy_options::recursive);
                } else {
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    if (cutMode) {
        clear();
    }
    return true;
}
