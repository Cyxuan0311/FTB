#include "../include/FTB/ClipboardManager.hpp"


/**
 * 向剪贴板添加项目路径
 * @param path 要添加的文件/目录路径
 * 
 * 功能说明：
 * 1. 检查路径是否已存在剪贴板中
 * 2. 如果不存在则添加
 * 3. 避免重复添加相同路径
 */
void ClipboardManager::addItem(const std::string& path) {
    // 检查路径是否已在剪贴板项目中
    if (std::find(items.begin(), items.end(), path) == items.end()) {
        // 添加新路径到剪贴板
        items.push_back(path);
    }
}


/**
 * 执行粘贴操作
 * @param targetPath 目标路径
 * @return 操作成功返回true，失败返回false
 * 
 * 功能说明：
 * 1. 检查剪贴板是否为空
 * 2. 根据剪切/复制模式执行不同操作
 * 3. 处理文件和目录的不同情况
 * 4. 剪切模式下粘贴后清空剪贴板
 */
bool ClipboardManager::paste(const std::string& targetPath) {
    // 检查剪贴板是否为空
    if (items.empty()) return false;
    
    // 遍历剪贴板中的所有项目
    for (const auto& sourcePath : items) {
        // 构造源路径和目标路径
        fs::path src(sourcePath);
        fs::path dst = fs::path(targetPath) / src.filename();
        
        try {
            // 根据剪切/复制模式执行不同操作
            if (cutMode) {
                // 剪切模式：移动文件/目录
                fs::rename(src, dst);
            } else {
                // 复制模式：复制文件/目录
                if (fs::is_directory(src)) {
                    // 递归复制目录
                    fs::copy(src, dst, fs::copy_options::recursive);
                } else {
                    // 复制文件（覆盖已存在的文件）
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const std::exception& e) {
            // 捕获并处理异常
            return false;
        }
    }
    
    // 如果是剪切模式，粘贴后清空剪贴板
    if (cutMode) {
        clear();
    }
    return true;
}

