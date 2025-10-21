#include "../../include/UI/JumpFileContextDialog.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/component.hpp>
#include <algorithm>
#include <sstream>
#include <filesystem>

using namespace ftxui;

namespace UI {

JumpFileContextDialog::JumpFileContextDialog()
    : target_path_("")
    , current_path_("")
    , use_absolute_path_(false)
    , create_if_not_exists_(false)
    , validate_path_(true)
    , dialog_open_(false)
    , confirmed_(false)
    , screen_(nullptr)
{
    // 创建输入框组件
    target_path_input_ = Input(&target_path_, "输入目标目录路径");
    current_path_display_ = Input(&current_path_, "当前路径（只读）");
    
    // 创建复选框组件
    use_absolute_checkbox_ = Checkbox("使用绝对路径", &use_absolute_path_);
    create_checkbox_ = Checkbox("目录不存在时创建", &create_if_not_exists_);
    validate_checkbox_ = Checkbox("验证路径有效性", &validate_path_);
    
    // 创建按钮组件
    confirm_button_ = Button("✅ 跳转", [this] { onConfirm(); });
    cancel_button_ = Button("❌ 取消", [this] { onCancel(); });
    
    // 初始化状态文本
    status_text_ = "";
}

Component JumpFileContextDialog::createDialogComponent() {
    // 创建表单组件 - 使用Container::Vertical确保正确的焦点管理
    auto form = Container::Vertical({
        target_path_input_,
        current_path_display_,
        use_absolute_checkbox_,
        create_checkbox_,
        validate_checkbox_
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
    main_container->Add(target_path_input_);
    main_container->Add(current_path_display_);
    main_container->Add(use_absolute_checkbox_);
    main_container->Add(create_checkbox_);
    main_container->Add(validate_checkbox_);
    main_container->Add(confirm_button_);
    main_container->Add(cancel_button_);
    
    return main_container;
}

FTB::JumpFileContextParams JumpFileContextDialog::showDialog(ScreenInteractive& screen) {
    dialog_open_ = true;
    confirmed_ = false;
    screen_ = &screen;
    
    // 获取当前工作目录
    current_path_ = std::filesystem::current_path().string();
    
    // 重置输入值
    resetInputs();
    
    // 创建对话框组件
    auto dialog_component = createDialogComponent();
    
    // 创建对话框渲染函数
    auto dialog_renderer = Renderer(dialog_component, [this] {
        auto title = text("📁 目录跳转") | bold | center | color(Color::Magenta);
        
        auto current_path_label = text("📍 当前路径:");
        auto current_path_field = current_path_display_->Render() | border | size(WIDTH, GREATER_THAN, 50) | color(Color::GrayLight);
        
        auto target_path_label = text("🎯 目标路径:");
        auto target_path_field = target_path_input_->Render() | border | size(WIDTH, GREATER_THAN, 50);
        
        auto absolute_label = text("🔗 路径类型:");
        auto absolute_checkbox = use_absolute_checkbox_->Render();
        
        auto create_label = text("📂 目录选项:");
        auto create_checkbox = create_checkbox_->Render();
        
        auto validate_label = text("✅ 验证选项:");
        auto validate_checkbox = validate_checkbox_->Render();
        
        auto buttons = hbox({
            confirm_button_->Render() | bgcolor(Color::Magenta) | color(Color::White) | bold | size(WIDTH, GREATER_THAN, 15),
            cancel_button_->Render() | bgcolor(Color::Red) | color(Color::White) | size(WIDTH, GREATER_THAN, 15)
        }) | center;
        
        // 根据状态文本内容设置颜色
        Color status_color = Color::Magenta;
        if (status_text_.find("错误:") != std::string::npos) {
            status_color = Color::Red;
        } else if (status_text_.find("验证通过") != std::string::npos) {
            status_color = Color::Green;
        } else if (status_text_.find("跳转成功") != std::string::npos) {
            status_color = Color::Green;
        }
        auto status = text(status_text_) | color(status_color);
        
        // 创建表单元素
        auto form = vbox({
            current_path_label,
            current_path_field,
            separator(),
            target_path_label,
            target_path_field,
            separator(),
            absolute_label,
            absolute_checkbox,
            create_label,
            create_checkbox,
            validate_label,
            validate_checkbox
        }) | flex;
        
        // 使用紫色主题的对话框
        auto content = vbox({
            title,
            separator(),
            form,
            separator(),
            buttons,
            status
        }) | border | bgcolor(Color::Black) | color(Color::Magenta) | size(WIDTH, GREATER_THAN, 70) | size(HEIGHT, GREATER_THAN, 25);
        
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
        FTB::JumpFileContextParams params;
        params.target_path = target_path_;
        params.use_absolute_path = use_absolute_path_;
        params.create_if_not_exists = create_if_not_exists_;
        params.validate_path = validate_path_;
        return params;
    } else {
        return FTB::JumpFileContextParams{};
    }
}

void JumpFileContextDialog::onConfirm() {
    if (validateInput()) {
        confirmed_ = true;
        dialog_open_ = false;
        if (screen_) {
            screen_->Exit();
        }
    }
}

void JumpFileContextDialog::onCancel() {
    confirmed_ = false;
    dialog_open_ = false;
    if (screen_) {
        screen_->Exit();
    }
}

bool JumpFileContextDialog::validateInput() {
    // 验证目标路径
    if (target_path_.empty()) {
        status_text_ = "错误: 目标路径不能为空";
        return false;
    }
    
    // 如果设置了验证回调，使用自定义验证
    if (validation_callback_) {
        if (!validation_callback_(target_path_)) {
            status_text_ = "错误: 路径验证失败";
            return false;
        }
    }
    
    // 基本路径验证
    try {
        std::filesystem::path target_path(target_path_);
        
        // 如果是绝对路径验证
        if (use_absolute_path_ && !target_path.is_absolute()) {
            status_text_ = "错误: 选择了绝对路径但输入的是相对路径";
            return false;
        }
        
        // 如果不需要创建且路径不存在
        if (!create_if_not_exists_ && validate_path_) {
            if (!std::filesystem::exists(target_path)) {
                status_text_ = "错误: 目标路径不存在";
                return false;
            }
            if (!std::filesystem::is_directory(target_path)) {
                status_text_ = "错误: 目标路径不是目录";
                return false;
            }
        }
        
    } catch (const std::exception& e) {
        status_text_ = "错误: 路径格式无效 - " + std::string(e.what());
        return false;
    }
    
    status_text_ = "验证通过，准备跳转...";
    return true;
}

void JumpFileContextDialog::resetInputs() {
    target_path_ = "";
    use_absolute_path_ = false;
    create_if_not_exists_ = false;
    validate_path_ = true;
    status_text_ = "";
}

void JumpFileContextDialog::setJumpCallback(std::function<void(const FTB::JumpFileContextParams&)> callback) {
    jump_callback_ = callback;
}

void JumpFileContextDialog::setValidationCallback(std::function<bool(const std::string&)> callback) {
    validation_callback_ = callback;
}

} // namespace UI
