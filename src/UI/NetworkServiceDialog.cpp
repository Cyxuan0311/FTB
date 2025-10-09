#include "../../include/UI/NetworkServiceDialog.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/component.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace ftxui;

namespace UI {

NetworkServiceDialog::NetworkServiceDialog() 
    : dialog_open_(false), auto_refresh_(false), active_tab_(0),
      refresh_interval_(5), show_detailed_info_(true),
      show_performance_metrics_(true), show_network_details_(true),
      last_speed_test_result_(0.0), speed_test_status_("未测试") {
    
    // 初始化网络服务
    network_service_ = std::make_unique<FTB::NetworkService>();
    connections_ = network_service_->getConnectionInfo();
    statistics_ = network_service_->getStatistics();
    
    // 初始化按钮
    refresh_button_ = Button("🔄 刷新", [this] { onRefresh(); });
    connect_button_ = Button("🔗 连接", [this] { onConnect(); });
    disconnect_button_ = Button("🔌 断开", [this] { onDisconnect(); });
    close_button_ = Button("❌ 关闭", [this] { onClose(); });
    speed_test_button_ = Button("⚡ 速度测试", [this] { onSpeedTest(); });
    auto_refresh_toggle_ = Button("⏰ 自动刷新", [this] { auto_refresh_ = !auto_refresh_; });
    
    // 初始化状态
    status_text_ = "网络服务已加载";
    last_update_time_ = network_service_->getLastUpdateTime();
}

void NetworkServiceDialog::showDialog(ftxui::ScreenInteractive& screen) {
    screen_ = &screen;
    dialog_open_ = true;
    
    // 创建主对话框
    auto main_dialog = createMainDialog();
    
    // 创建对话框渲染器
    auto dialog_renderer = Renderer(main_dialog, [this, main_dialog] {
        return vbox({
            // 标题栏
            text("🌐 网络服务管理器") | bold | color(Color::Blue) | center | bgcolor(Color::DarkBlue),
            separator(),
            
            // 主内容区域 - 限制大小并居中
            main_dialog->Render() | size(WIDTH, EQUAL, 80) | size(HEIGHT, EQUAL, 20) | center,
            
            separator(),
            
            // 状态栏
            hbox({
                text("状态: " + status_text_) | color(Color::Green),
                filler(),
                text("最后更新: " + last_update_time_) | color(Color::GrayLight),
                text(" | "),
                text("按ESC关闭") | color(Color::Yellow)
            }) | color(Color::DarkBlue) | bgcolor(Color::GrayDark)
        }) | borderDouble | color(Color::Blue);
    });
    
    // 设置事件处理 - 支持键盘导航
    auto event_handler = CatchEvent([this](Event event) {
        if (event == Event::Escape) {
            dialog_open_ = false;
            if (screen_) {
                screen_->Exit();
            }
            return true;
        }
        if (event == Event::F5) {
            onRefresh();
            return true;
        }
        if (event == Event::CtrlR) {
            onRefresh();
            return true;
        }
        if (event == Event::Tab) {
            active_tab_ = (active_tab_ + 1) % 4; // 循环切换标签页
            return true;
        }
        if (event == Event::Character('1')) {
            active_tab_ = 0; // 连接信息
            return true;
        }
        if (event == Event::Character('2')) {
            active_tab_ = 1; // 统计信息
            return true;
        }
        if (event == Event::Character('3')) {
            active_tab_ = 2; // 速度测试
            return true;
        }
        if (event == Event::Character('4')) {
            active_tab_ = 3; // 设置
            return true;
        }
        // 允许鼠标事件用于按钮交互
        return false;
    });
    
    auto final_component = event_handler(dialog_renderer);
    
    // 显示对话框
    screen.Loop(final_component);
}

ftxui::Component NetworkServiceDialog::createMainDialog() {
    // 创建现代化的标签页界面
    return createModernTabbedInterface();
}

// 创建现代化标签页界面
ftxui::Component NetworkServiceDialog::createModernTabbedInterface() {
    // 创建标签页组件
    auto connection_tab = createConnectionTab();
    auto statistics_tab = createStatisticsTab();
    auto speed_test_tab = createSpeedTestTab();
    auto settings_tab = createSettingsTab();
    
    // 创建标签页容器
    std::vector<std::string> tab_names = {"🔗 连接信息", "📊 统计信息", "⚡ 速度测试", "⚙️ 设置"};
    std::vector<ftxui::Component> tab_components = {connection_tab, statistics_tab, speed_test_tab, settings_tab};
    
    auto tab_container = Container::Tab(tab_components, &active_tab_);
    
    // 创建顶部工具栏
    auto toolbar_renderer = Renderer([this] {
        return hbox({
            refresh_button_->Render() | color(Color::Green),
            text(" | "),
            connect_button_->Render() | color(Color::Blue),
            text(" | "),
            disconnect_button_->Render() | color(Color::Red),
            text(" | "),
            speed_test_button_->Render() | color(Color::Yellow),
            text(" | "),
            auto_refresh_toggle_->Render() | color(Color::Cyan),
            text(" | "),
            close_button_->Render() | color(Color::GrayLight)
        }) | border | color(Color::DarkBlue);
    });
    
    // 组合所有组件
    return Container::Vertical({
        toolbar_renderer,
        tab_container
    });
}

// 创建连接信息标签页
ftxui::Component NetworkServiceDialog::createConnectionTab() {
    return Renderer([this] {
        return vbox({
            text("🔗 网络连接信息") | bold | color(Color::Blue) | center,
            separator(),
            
            // 连接状态概览
            hbox({
                text("状态: ") | color(Color::Cyan),
                text(network_service_->getStatus() == FTB::NetworkStatus::CONNECTED ? "已连接" : "未连接") | 
                color(network_service_->getStatus() == FTB::NetworkStatus::CONNECTED ? Color::Green : Color::Red)
            }),
            
            separator(),
            
            // 连接列表
            text("📡 可用连接:") | bold | color(Color::Yellow),
            [this] {
                Elements connection_elements;
                for (const auto& conn : connections_) {
                    connection_elements.push_back(
                        vbox({
                            hbox({
                                text("📶 " + conn.name) | bold | color(Color::White),
                                text(" (" + conn.type + ")") | color(Color::GrayLight)
                            }),
                            hbox({
                                text("  IP: " + conn.ip_address) | color(Color::Green),
                                text(" | MAC: " + conn.mac_address) | color(Color::Cyan)
                            }),
                            hbox({
                                text("  状态: " + conn.status) | color(Color::Yellow),
                                text(" | 信号: " + std::to_string(conn.signal_strength) + "%") | color(Color::Magenta)
                            }),
                            text("")
                        })
                    );
                }
                return vbox(connection_elements);
            }()
        });
    });
}

// 创建统计信息标签页
ftxui::Component NetworkServiceDialog::createStatisticsTab() {
    return Renderer([this] {
        return vbox({
            text("📊 网络统计信息") | bold | color(Color::Blue) | center,
            separator(),
            
            // 数据传输统计
            text("📈 数据传输") | bold | color(Color::Yellow),
            hbox({
                text("发送: " + std::to_string(statistics_.bytes_sent / 1024) + " KB") | color(Color::Green),
                text(" | "),
                text("接收: " + std::to_string(statistics_.bytes_received / 1024) + " KB") | color(Color::Cyan)
            }),
            
            hbox({
                text("发送包: " + std::to_string(statistics_.packets_sent)) | color(Color::Green),
                text(" | "),
                text("接收包: " + std::to_string(statistics_.packets_received)) | color(Color::Cyan)
            }),
            
            separator(),
            
            // 速度信息
            text("⚡ 网络速度") | bold | color(Color::Yellow),
            hbox({
                text("上传: " + std::to_string(static_cast<int>(statistics_.upload_speed)) + " KB/s") | color(Color::Green),
                text(" | "),
                text("下载: " + std::to_string(static_cast<int>(statistics_.download_speed)) + " KB/s") | color(Color::Cyan)
            }),
            
            separator(),
            
            // 运行时间
            text("⏱️ 运行时间: " + statistics_.uptime) | color(Color::Magenta)
        });
    });
}

// 创建速度测试标签页
ftxui::Component NetworkServiceDialog::createSpeedTestTab() {
    return Renderer([this] {
        return vbox({
            text("⚡ 网络速度测试") | bold | color(Color::Blue) | center,
            separator(),
            
            // 测试状态
            hbox({
                text("测试状态: ") | color(Color::Cyan),
                text(speed_test_status_) | color(Color::Yellow)
            }),
            
            separator(),
            
            // 测试结果
            [this] {
                if (last_speed_test_result_ > 0) {
                    return vbox({
                        text("📊 测试结果") | bold | color(Color::Yellow),
                        hbox({
                            text("速度: " + std::to_string(static_cast<int>(last_speed_test_result_)) + " Mbps") | color(Color::Green)
                        }),
                        text("测试时间: " + last_update_time_) | color(Color::GrayLight)
                    });
                } else {
                    return text("点击'速度测试'按钮开始测试") | color(Color::GrayLight) | center;
                }
            }(),
            
            separator(),
            
            // 测试说明
            text("💡 测试说明:") | bold | color(Color::Yellow),
            text("• 测试连接到Google DNS (8.8.8.8)") | color(Color::White),
            text("• 结果仅供参考，实际速度可能因网络环境而异") | color(Color::GrayLight)
        });
    });
}

// 创建设置标签页
ftxui::Component NetworkServiceDialog::createSettingsTab() {
    return Renderer([this] {
        return vbox({
            text("⚙️ 网络设置") | bold | color(Color::Blue) | center,
            separator(),
            
            // 刷新设置
            text("🔄 自动刷新设置") | bold | color(Color::Yellow),
            hbox({
                text("刷新间隔: " + std::to_string(refresh_interval_) + " 秒") | color(Color::White),
                text(" | "),
                text(auto_refresh_ ? "已启用" : "已禁用") | color(auto_refresh_ ? Color::Green : Color::Red)
            }),
            
            separator(),
            
            // 显示选项
            text("👁️ 显示选项") | bold | color(Color::Yellow),
            hbox({
                text("详细信息: " + std::string(show_detailed_info_ ? "显示" : "隐藏")) | color(Color::White),
                text(" | "),
                text("性能指标: " + std::string(show_performance_metrics_ ? "显示" : "隐藏")) | color(Color::White)
            }),
            
            separator(),
            
            // 快捷键说明
            text("⌨️ 快捷键") | bold | color(Color::Yellow),
            text("Tab: 切换标签页") | color(Color::White),
            text("1-4: 直接跳转到对应标签页") | color(Color::White),
            text("F5/Ctrl+R: 刷新信息") | color(Color::White),
            text("ESC: 关闭对话框") | color(Color::White)
        });
    });
}

// 创建连接信息面板
ftxui::Component NetworkServiceDialog::createConnectionInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("连接详细信息") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建统计信息面板
ftxui::Component NetworkServiceDialog::createStatisticsPanel() {
    return Renderer([this] {
        return vbox({
            text("网络统计监控") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建速度测试面板
ftxui::Component NetworkServiceDialog::createSpeedTestPanel() {
    return Renderer([this] {
        return vbox({
            text("网络速度测试") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建设置面板
ftxui::Component NetworkServiceDialog::createSettingsPanel() {
    return Renderer([this] {
        return vbox({
            text("网络服务设置") | bold | color(Color::Yellow),
            separator()
        });
    });
}

void NetworkServiceDialog::onRefresh() {
    status_text_ = "正在刷新网络信息...";
    connections_ = network_service_->getConnectionInfo();
    statistics_ = network_service_->getStatistics();
    last_update_time_ = network_service_->getLastUpdateTime();
    status_text_ = "网络信息已更新";
    
    if (status_update_callback_) {
        status_update_callback_();
    }
}

void NetworkServiceDialog::onConnect() {
    status_text_ = "正在连接网络...";
    // 这里可以添加连接逻辑
    status_text_ = "连接操作已执行";
}

void NetworkServiceDialog::onDisconnect() {
    status_text_ = "正在断开网络连接...";
    if (network_service_->disconnectNetwork()) {
        status_text_ = "网络连接已断开";
    } else {
        status_text_ = "断开连接失败";
    }
}

void NetworkServiceDialog::onClose() {
    dialog_open_ = false;
    if (screen_) {
        screen_->Exit();
    }
}

void NetworkServiceDialog::onSpeedTest() {
    status_text_ = "正在进行速度测试...";
    speed_test_status_ = "测试中...";
    
    // 执行速度测试
    double speed = network_service_->getNetworkSpeed();
    last_speed_test_result_ = speed;
    speed_test_status_ = "测试完成";
    status_text_ = "速度测试完成";
}

} // namespace UI