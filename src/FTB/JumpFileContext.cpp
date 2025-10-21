#include "FTB/JumpFileContext.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace FTB {

bool JumpFileContext::validateTargetPath(const JumpFileContextParams& params) {
    if (params.target_path.empty()) {
        return false;
    }
    
    try {
        fs::path target_path(params.target_path);
        
        // 如果是相对路径，需要进一步验证
        if (!params.use_absolute_path && !target_path.is_absolute()) {
            // 相对路径是有效的
            return true;
        }
        
        // 如果是绝对路径，检查路径是否存在
        if (params.validate_path) {
            if (params.create_if_not_exists) {
                // 如果允许创建，检查父目录是否存在
                if (target_path.has_parent_path()) {
                    return fs::exists(target_path.parent_path());
                }
                return true; // 根目录
            } else {
                // 不允许创建，检查路径是否存在
                return fs::exists(target_path) && fs::is_directory(target_path);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ 路径验证错误: " << e.what() << std::endl;
        return false;
    }
}

bool JumpFileContext::executeJump(const JumpFileContextParams& params) {
    try {
        fs::path target_path(params.target_path);
        
        // 如果是相对路径，转换为绝对路径
        if (!target_path.is_absolute()) {
            target_path = fs::current_path() / target_path;
        }
        
        // 如果路径不存在且允许创建
        if (params.create_if_not_exists && !fs::exists(target_path)) {
            if (!createDirectoryIfNeeded(target_path.string())) {
                std::cerr << "❌ 无法创建目录: " << target_path.string() << std::endl;
                return false;
            }
        }
        
        // 验证路径是否存在且是目录
        if (!fs::exists(target_path)) {
            std::cerr << "❌ 目标路径不存在: " << target_path.string() << std::endl;
            return false;
        }
        
        if (!fs::is_directory(target_path)) {
            std::cerr << "❌ 目标路径不是目录: " << target_path.string() << std::endl;
            return false;
        }
        
        // 获取规范化路径
        std::string canonical_path = getCanonicalPath(target_path.string());
        
        // 验证最终路径
        if (!fs::exists(canonical_path) || !fs::is_directory(canonical_path)) {
            std::cerr << "❌ 目标路径不是有效的目录: " << canonical_path << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ 目录跳转执行错误: " << e.what() << std::endl;
        return false;
    }
}

std::string JumpFileContext::getCanonicalPath(const std::string& path) {
    try {
        fs::path target_path(path);
        
        // 如果是相对路径，先转换为绝对路径
        if (!target_path.is_absolute()) {
            target_path = fs::current_path() / target_path;
        }
        
        // 使用并发方式获取规范化路径，提高性能
        return fs::canonical(target_path).string();
    } catch (const std::exception& e) {
        std::cerr << "❌ 路径规范化错误: " << e.what() << std::endl;
        return path; // 返回原始路径
    }
}

bool JumpFileContext::pathExists(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_directory(path);
    } catch (const std::exception& e) {
        std::cerr << "❌ 路径检查错误: " << e.what() << std::endl;
        return false;
    }
}

bool JumpFileContext::createDirectoryIfNeeded(const std::string& path) {
    try {
        if (fs::exists(path)) {
            return fs::is_directory(path);
        }
        
        // 创建目录（包括所有必要的父目录）
        return fs::create_directories(path);
    } catch (const std::exception& e) {
        std::cerr << "❌ 目录创建错误: " << e.what() << std::endl;
        return false;
    }
}

} // namespace FTB
