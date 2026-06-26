#pragma once

#include <string>
#include <filesystem>

namespace FTB {

/**
 * @brief 跳转目录参数结构体
 * 用于存储用户输入的目录跳转信息
 */
struct JumpFileContextParams {
    std::string target_path;     ///< 目标目录路径
    bool use_absolute_path;      ///< 是否使用绝对路径
    bool create_if_not_exists;   ///< 如果目录不存在是否创建
    bool validate_path;          ///< 是否验证路径有效性
    
    // 默认构造函数
    JumpFileContextParams() 
        : target_path("")
        , use_absolute_path(false)
        , create_if_not_exists(false)
        , validate_path(true) {}
    
    // 带参数的构造函数
    JumpFileContextParams(const std::string& path, bool absolute = false, bool create = false, bool validate = true)
        : target_path(path)
        , use_absolute_path(absolute)
        , create_if_not_exists(create)
        , validate_path(validate) {}
};

/**
 * @brief 跳转目录上下文类
 * 处理目录跳转相关的逻辑
 */
class JumpFileContext {
public:
    /**
     * @brief 验证目标路径是否有效
     * @param params 跳转参数
     * @return 如果路径有效返回true，否则返回false
     */
    static bool validateTargetPath(const JumpFileContextParams& params);
    
    /**
     * @brief 执行目录跳转
     * @param params 跳转参数
     * @return 如果跳转成功返回true，否则返回false
     */
    static bool executeJump(const JumpFileContextParams& params);
    
    /**
     * @brief 获取规范化路径
     * @param path 原始路径
     * @return 规范化后的路径
     */
    static std::string getCanonicalPath(const std::string& path);
    
    /**
     * @brief 检查路径是否存在
     * @param path 要检查的路径
     * @return 如果路径存在返回true，否则返回false
     */
    static bool pathExists(const std::string& path);
    
    /**
     * @brief 创建目录（如果不存在）
     * @param path 要创建的目录路径
     * @return 如果创建成功或已存在返回true，否则返回false
     */
    static bool createDirectoryIfNeeded(const std::string& path);
};

} // namespace FTB
