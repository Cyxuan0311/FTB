#ifndef SSH_DIALOG_HPP
#define SSH_DIALOG_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <functional>
#include "../../include/Connection/SSHConnection.hpp"

namespace UI {

// SSH连接对话框类
class SSHDialog {
public:
    SSHDialog();
    ~SSHDialog() = default;

    // 显示对话框并返回连接参数
    Connection::SSHConnectionParams showDialog(ftxui::ScreenInteractive& screen);
    
    // 设置连接回调
    void setConnectionCallback(std::function<void(const Connection::SSHConnectionParams&)> callback) {
        connection_callback_ = callback;
    }

private:
    // 创建对话框组件
    ftxui::Component createDialogComponent();
    
    // 处理确定按钮点击
    void onConfirm();
    
    // 处理取消按钮点击
    void onCancel();
    
    // 验证输入
    bool validateInput();

private:
    // 输入框组件
    ftxui::Component hostname_input_;
    ftxui::Component port_input_;
    ftxui::Component username_input_;
    ftxui::Component password_input_;
    ftxui::Component remote_dir_input_;
    ftxui::Component private_key_input_;
    
    // 复选框组件
    ftxui::Component use_key_auth_checkbox_;
    
    // 按钮组件
    ftxui::Component confirm_button_;
    ftxui::Component cancel_button_;
    
    // 状态文本
    std::string status_text_;
    
    // 输入值
    std::string hostname_;
    std::string port_;
    std::string username_;
    std::string password_;
    std::string remote_directory_;
    std::string private_key_path_;
    bool use_key_auth_;
    
    // 对话框状态
    bool dialog_open_;
    bool confirmed_;
    
    // 回调函数
    std::function<void(const Connection::SSHConnectionParams&)> connection_callback_;
    
    // 屏幕引用
    ftxui::ScreenInteractive* screen_;
};

} // namespace UI

#endif // SSH_DIALOG_HPP
