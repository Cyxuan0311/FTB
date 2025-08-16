#include "../../include/UI/SSHDialog.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/component.hpp>
#include <algorithm>
#include <sstream>

using namespace ftxui;

namespace UI {

SSHDialog::SSHDialog()
    : hostname_("")
    , port_("22")
    , username_("")
    , password_("")
    , remote_directory_("/home")
    , private_key_path_("")
    , use_key_auth_(false)
    , dialog_open_(false)
    , confirmed_(false)
    , screen_(nullptr)
{
    // 创建输入框组件
    hostname_input_ = Input(&hostname_, "主机名或IP地址");
    port_input_ = Input(&port_, "端口");
    username_input_ = Input(&username_, "用户名");
    password_input_ = Input(&password_, "密码");
    remote_dir_input_ = Input(&remote_directory_, "远程目录");
    private_key_input_ = Input(&private_key_path_, "私钥文件路径");
    
    // 创建复选框组件
    use_key_auth_checkbox_ = Checkbox("使用密钥认证", &use_key_auth_);
    
    // 创建按钮组件
    confirm_button_ = Button("确定", [this] { onConfirm(); });
    cancel_button_ = Button("取消", [this] { onCancel(); });
    
    // 创建状态文本组件
    status_text_ = Text("") | color(Color::Yellow);
}

ftxui::Component SSHDialog::createDialogComponent() {
    // 创建表单组件
    auto form = Container::Vertical({
        hostname_input_,
        port_input_,
        username_input_,
        password_input_,
        remote_dir_input_,
        use_key_auth_checkbox_,
        private_key_input_
    });
    
    // 创建按钮容器
    auto buttons = Container::Horizontal({
        confirm_button_,
        cancel_button_
    });
    
    // 创建主容器
    auto main_container = Container::Vertical({
        form,
        buttons,
        status_text_
    });
    
    // 设置焦点
    main_container->Add(hostname_input_);
    main_container->Add(port_input_);
    main_container->Add(username_input_);
    main_container->Add(password_input_);
    main_container->Add(remote_dir_input_);
    main_container->Add(use_key_auth_checkbox_);
    main_container->Add(private_key_input_);
    main_container->Add(confirm_button_);
    main_container->Add(cancel_button_);
    
    return main_container;
}

Connection::SSHConnectionParams SSHDialog::showDialog(ftxui::ScreenInteractive& screen) {
    dialog_open_ = true;
    confirmed_ = false;
    screen_ = &screen;
    
    // 重置输入值
    hostname_ = "";
    port_ = "22";
    username_ = "";
    password_ = "";
    remote_directory_ = "/home";
    private_key_path_ = "";
    use_key_auth_ = false;
    
    // 创建对话框组件
    auto dialog_component = createDialogComponent();
    
    // 创建对话框渲染函数
    auto dialog_renderer = Renderer(dialog_component, [this, dialog_component] {
        auto title = text("SSH 连接配置") | bold | center;
        
        auto hostname_label = text("主机名/IP地址:");
        auto hostname_field = hostname_input_->Render() | border;
        
        auto port_label = text("端口:");
        auto port_field = port_input_->Render() | border;
        
        auto username_label = text("用户名:");
        auto username_field = username_input_->Render() | border;
        
        auto password_label = text("密码:");
        auto password_field = password_input_->Render() | border;
        
        auto remote_dir_label = text("远程目录:");
        auto remote_dir_field = remote_dir_input_->Render() | border;
        
        auto auth_label = text("认证方式:");
        auto auth_checkbox = use_key_auth_checkbox_->Render();
        
        auto key_label = text("私钥文件路径:");
        auto key_field = private_key_input_->Render() | border;
        
        auto buttons = Container::Horizontal({
            confirm_button_->Render() | bgcolor(Color::Green),
            cancel_button_->Render() | bgcolor(Color::Red)
        }) | center;
        
        auto status = status_text_->Render();
        
        // 根据认证方式显示/隐藏相关字段
        Elements form_elements = {
            hostname_label,
            hostname_field,
            port_label,
            port_field,
            username_label,
            username_field,
            remote_dir_label,
            remote_dir_field,
            auth_label,
            auth_checkbox
        };
        
        if (use_key_auth_) {
            form_elements.push_back(key_label);
            form_elements.push_back(key_field);
        } else {
            form_elements.push_back(password_label);
            form_elements.push_back(password_field);
        }
        
        auto form = vbox(form_elements) | flex;
        
        auto content = vbox({
            title,
            separator(),
            form,
            separator(),
            buttons,
            status
        }) | border | bgcolor(Color::Black) | color(Color::White);
        
        return content | center;
    });
    
    // 设置事件处理
    auto event_handler = CatchEvent([this](Event event) {
        if (event == Event::Escape) {
            onCancel();
            return true;
        }
        return false;
    });
    
    auto final_component = event_handler(dialog_renderer);
    
    // 显示对话框
    screen.Loop(final_component);
    
    // 返回结果
    if (confirmed_) {
        Connection::SSHConnectionParams params;
        params.hostname = hostname_;
        params.port = std::stoi(port_);
        params.username = username_;
        params.password = password_;
        params.remote_directory = remote_directory_;
        params.private_key_path = private_key_path_;
        params.use_key_auth = use_key_auth_;
        return params;
    } else {
        return Connection::SSHConnectionParams{};
    }
}

void SSHDialog::onConfirm() {
    if (validateInput()) {
        confirmed_ = true;
        dialog_open_ = false;
        if (screen_) {
            screen_->Exit();
        }
    }
}

void SSHDialog::onCancel() {
    confirmed_ = false;
    dialog_open_ = false;
    if (screen_) {
        screen_->Exit();
    }
}

bool SSHDialog::validateInput() {
    // 验证主机名
    if (hostname_.empty()) {
        status_text_ = Text("错误: 主机名不能为空") | color(Color::Red);
        return false;
    }
    
    // 验证端口
    try {
        int port = std::stoi(port_);
        if (port <= 0 || port > 65535) {
            status_text_ = Text("错误: 端口号必须在1-65535之间") | color(Color::Red);
            return false;
        }
    } catch (const std::exception&) {
        status_text_ = Text("错误: 端口号必须是有效的数字") | color(Color::Red);
        return false;
    }
    
    // 验证用户名
    if (username_.empty()) {
        status_text_ = Text("错误: 用户名不能为空") | color(Color::Red);
        return false;
    }
    
    // 验证认证信息
    if (use_key_auth_) {
        if (private_key_path_.empty()) {
            status_text_ = Text("错误: 使用密钥认证时私钥路径不能为空") | color(Color::Red);
            return false;
        }
    } else {
        if (password_.empty()) {
            status_text_ = Text("错误: 使用密码认证时密码不能为空") | color(Color::Red);
            return false;
        }
    }
    
    // 验证远程目录
    if (remote_directory_.empty()) {
        status_text_ = Text("错误: 远程目录不能为空") | color(Color::Red);
        return false;
    }
    
    status_text_ = Text("验证通过，正在连接...") | color(Color::Green);
    return true;
}

} // namespace UI
