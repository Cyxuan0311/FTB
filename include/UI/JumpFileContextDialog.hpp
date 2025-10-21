#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <functional>
#include "FTB/JumpFileContext.hpp"

using namespace ftxui;

namespace UI {

/**
 * @brief 跳转目录对话框类
 * 提供用户输入目录路径的界面，参考SSHDialog的设计
 */
class JumpFileContextDialog {
public:
    /**
     * @brief 构造函数
     */
    JumpFileContextDialog();
    
    /**
     * @brief 析构函数
     */
    ~JumpFileContextDialog() = default;
    
    /**
     * @brief 显示对话框并获取用户输入
     * @param screen FTXUI交互式屏幕引用
     * @return 用户输入的跳转参数
     */
    FTB::JumpFileContextParams showDialog(ScreenInteractive& screen);
    
    /**
     * @brief 设置跳转回调函数
     * @param callback 跳转成功时的回调函数
     */
    void setJumpCallback(std::function<void(const FTB::JumpFileContextParams&)> callback);
    
    /**
     * @brief 设置验证回调函数
     * @param callback 路径验证时的回调函数
     */
    void setValidationCallback(std::function<bool(const std::string&)> callback);

private:
    // 输入字段
    std::string target_path_;           ///< 目标路径
    std::string current_path_;          ///< 当前路径（显示用）
    bool use_absolute_path_;           ///< 是否使用绝对路径
    bool create_if_not_exists_;        ///< 如果不存在是否创建
    bool validate_path_;               ///< 是否验证路径
    
    // 状态
    bool dialog_open_;                 ///< 对话框是否打开
    bool confirmed_;                   ///< 是否确认
    std::string status_text_;          ///< 状态文本
    ScreenInteractive* screen_;        ///< 屏幕引用
    
    // 回调函数
    std::function<void(const FTB::JumpFileContextParams&)> jump_callback_;
    std::function<bool(const std::string&)> validation_callback_;
    
    // UI组件
    Component target_path_input_;      ///< 目标路径输入框
    Component current_path_display_;   ///< 当前路径显示
    Component use_absolute_checkbox_; ///< 绝对路径复选框
    Component create_checkbox_;       ///< 创建目录复选框
    Component validate_checkbox_;     ///< 验证路径复选框
    Component confirm_button_;        ///< 确认按钮
    Component cancel_button_;         ///< 取消按钮
    
    /**
     * @brief 创建对话框组件
     * @return 对话框组件
     */
    Component createDialogComponent();
    
    /**
     * @brief 确认按钮回调
     */
    void onConfirm();
    
    /**
     * @brief 取消按钮回调
     */
    void onCancel();
    
    /**
     * @brief 验证用户输入
     * @return 如果输入有效返回true，否则返回false
     */
    bool validateInput();
    
    /**
     * @brief 重置输入值
     */
    void resetInputs();
};

} // namespace UI
