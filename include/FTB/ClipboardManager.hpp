#ifndef CLIPBOARD_MANAGER_HPP
#define CLIPBOARD_MANAGER_HPP

#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class ClipboardManager {
public:
    static ClipboardManager& getInstance() {
        static ClipboardManager instance;
        return instance;
    }

    void addItem(const std::string& path);
    void clear() {
        items.clear();
        cutMode = false;
        modeSelected = false;  // 清空时重置标记
    }
    bool isCutMode() const { return cutMode; }
    void setCutMode(bool mode) { 
        cutMode = mode;
        modeSelected = true; 
    }
    const std::vector<std::string>& getItems() const { return items; }
    bool hasModeSelected() const { return modeSelected; }  // 新增：检查是否已选择模式
    bool paste(const std::string& targetPath);

private:
    ClipboardManager() = default;
    std::vector<std::string> items;
    bool cutMode = false;
    bool modeSelected = false; 
};

#endif // CLIPBOARD_MANAGER_HPP