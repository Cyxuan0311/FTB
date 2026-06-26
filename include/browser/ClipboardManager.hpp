#ifndef CLIPBOARD_MANAGER_HPP
#define CLIPBOARD_MANAGER_HPP

#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;

class ClipboardManager {
public:
    static ClipboardManager& getInstance();

    void addItem(const std::string& path);
    void clear();
    bool isCutMode() const { return cutMode; }
    void setCutMode(bool mode) {
        cutMode = mode;
        modeSelected = true;
    }
    const std::vector<std::string>& getItems() const { return items; }
    bool hasModeSelected() const { return modeSelected; }

    // Async paste — submits to TaskSystem, returns immediately
    // force_overwrite=true: overwrite existing files, false: auto-rename
    std::string paste(const std::string& targetPath, bool force_overwrite = false);

private:
    ClipboardManager() = default;
    std::vector<std::string> items;
    bool cutMode = false;
    bool modeSelected = false;
};

#endif
