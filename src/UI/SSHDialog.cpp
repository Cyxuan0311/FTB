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
    confirm_button_ = Button("✅ 确定", [this] { onConfirm(); });
    cancel_button_ = Button("❌ 取消", [this] { onCancel(); });
    
    // 初始化状态文本
    status_text_ = "";
}

ftxui::Component SSHDialog::createDialogComponent() {
    // 创建表单组件 - 使用Container::Vertical确保正确的焦点管理
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
    
    // 创建主容器 - 垂直排列表单和按钮
    auto main_container = Container::Vertical({
        form,
        buttons
    });
    
    // 设置焦点顺序 - 这是关键步骤
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
    status_text_ = "";
    
    // 创建对话框组件
    auto dialog_component = createDialogComponent();
    
    // 创建对话框渲染函数
    auto dialog_renderer = Renderer(dialog_component, [this] {
        auto title = text("🔗 SSH 连接配置") | bold | center;
        
        auto hostname_label = text("🌐 主机名/IP地址:");
        auto hostname_field = hostname_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto port_label = text("🔌 端口:");
        auto port_field = port_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto username_label = text("👤 用户名:");
        auto username_field = username_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto password_label = text("🔒 密码:");
        auto password_field = password_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto remote_dir_label = text("📁 远程目录:");
        auto remote_dir_field = remote_dir_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto auth_label = text("🔐 认证方式:");
        auto auth_checkbox = use_key_auth_checkbox_->Render();
        
        auto key_label = text("🗝️ 私钥文件路径:");
        auto key_field = private_key_input_->Render() | border | size(WIDTH, GREATER_THAN, 40);
        
        auto buttons = hbox({
            confirm_button_->Render() | bgcolor(Color::Green) | size(WIDTH, GREATER_THAN, 15),
            cancel_button_->Render() | bgcolor(Color::Red) | size(WIDTH, GREATER_THAN, 15)
        }) | center;
        
        // 根据状态文本内容设置颜色
        auto status_color = Color::Yellow;
        if (status_text_.find("错误:") != std::string::npos) {
            status_color = Color::Red;
        } else if (status_text_.find("验证通过") != std::string::npos) {
            status_color = Color::Green;
        }
        auto status = text(status_text_) | color(status_color);
        
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
        }) | border | bgcolor(Color::Black) | color(Color::Blue) | size(WIDTH, GREATER_THAN, 60) | size(HEIGHT, GREATER_THAN, 20);
        
        return content | center;
    });
    
    // 设置事件处理 - 只处理ESC键，禁用鼠标，其他事件传递给子组件
    auto event_handler = CatchEvent([this](Event event) {
        if (event == Event::Escape) {
            onCancel();
            return true;
        }
        // 禁用所有鼠标事件
        if (event.is_mouse()) {
            return true; // 拦截所有鼠标事件
        }
        // 允许其他事件传递给子组件
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
        status_text_ = "错误: 主机名不能为空";
        return false;
    }
    
    // 验证端口
    try {
        int port = std::stoi(port_);
        if (port <= 0 || port > 65535) {
            status_text_ = "错误: 端口号必须在1-65535之间";
            return false;
        }
    } catch (const std::exception&) {
        status_text_ = "错误: 端口号必须是有效的数字";
        return false;
    }
    
    // 验证用户名
    if (username_.empty()) {
        status_text_ = "错误: 用户名不能为空";
        return false;
    }
    
    // 验证认证信息
    if (use_key_auth_) {
        if (private_key_path_.empty()) {
            status_text_ = "错误: 使用密钥认证时私钥路径不能为空";
            return false;
        }
    } else {
        if (password_.empty()) {
            status_text_ = "错误: 使用密码认证时密码不能为空";
            return false;
        }
    }
    
    // 验证远程目录
    if (remote_directory_.empty()) {
        status_text_ = "错误: 远程目录不能为空";
        return false;
    }
    
    status_text_ = "验证通过，正在连接...";
    return true;
}

} // namespace UI
