#include "../../include/UI/SystemInfoDialog.hpp"
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

SystemInfoDialog::SystemInfoDialog() 
    : dialog_open_(false), auto_refresh_(false), active_tab_(0),
      refresh_interval_(5), export_format_("text"), show_detailed_info_(true),
      show_performance_metrics_(true), show_network_details_(true) {
    
    // 初始化系统信息收集器
    info_collector_ = std::make_unique<FTB::SystemInfoCollector>();
    system_info_data_ = info_collector_->collectAllInfo();
    
    // 初始化按钮
    refresh_button_ = Button("🔄 刷新", [this] { onRefresh(); });
    export_button_ = Button("📤 导出", [this] { onExport(); });
    close_button_ = Button("❌ 关闭", [this] { onClose(); });
    auto_refresh_toggle_ = Button("⏰ 自动刷新", [this] { auto_refresh_ = !auto_refresh_; });
    
    // 初始化状态
    status_text_ = "系统信息已加载";
    last_update_time_ = info_collector_->getLastUpdateTime();
}

void SystemInfoDialog::showDialog(ftxui::ScreenInteractive& screen) {
    screen_ = &screen;
    dialog_open_ = true;
    
    // 创建现代化的多面板界面
    auto modern_component = createMainDialog();
    
    // 创建对话框渲染函数 - 重新设计样式
    auto dialog_renderer = Renderer(modern_component, [this, modern_component] {
        auto title = text("🔧 系统信息管理器") | bold | center | color(Color::Blue);
        
        // 创建主内容区域
        auto main_content = modern_component->Render() | flex;
        
        // 创建状态信息
        auto status_color = Color::Green;
        if (status_text_.find("错误:") != std::string::npos) {
            status_color = Color::Red;
        } else if (status_text_.find("已刷新") != std::string::npos) {
            status_color = Color::Yellow;
        }
        auto status = text(status_text_) | color(status_color);
        
        // 创建操作提示
        auto help_text = text("💡 按 ESC 退出 | F5 刷新 | Ctrl+E 导出 | 自动刷新: " + 
                             std::string(auto_refresh_ ? "开启" : "关闭")) | 
                         color(auto_refresh_ ? Color::Green : Color::Red) | center;
        
        // 组合所有内容
        auto content = vbox({
            title,
            separator(),
            main_content,
            separator(),
            status,
            help_text
        }) | border | bgcolor(Color::Black) | color(Color::White) | 
           size(WIDTH, GREATER_THAN, 100) | size(HEIGHT, GREATER_THAN, 30);
        
        return content | center;
    });
    
    // 设置事件处理 - 支持更多快捷键
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
        if (event == Event::CtrlE) {
            onExport();
            return true;
        }
        // 允许鼠标事件用于按钮交互
        return false;
    });
    
    auto final_component = event_handler(dialog_renderer);
    
    // 显示对话框
    screen.Loop(final_component);
}

ftxui::Component SystemInfoDialog::createMainDialog() {
    // 创建现代化的标签页界面
    return createModernTabbedInterface();
}

// 创建现代化标签页界面
ftxui::Component SystemInfoDialog::createModernTabbedInterface() {
    // 创建标签页组件
    auto device_tab = createDeviceInfoTab();
    auto status_tab = createStatusInfoTab();
    auto disk_tab = createDiskInfoTab();
    auto network_tab = createNetworkInfoTab();
    auto system_tab = createSystemInfoTab();
    
    // 创建标签页容器
    std::vector<std::string> tab_names = {"🖥️ 设备信息", "📊 状态信息", "💾 磁盘信息", "🌐 网络信息", "⚙️ 系统信息"};
    std::vector<ftxui::Component> tab_components = {device_tab, status_tab, disk_tab, network_tab, system_tab};
    
    auto tab_container = Container::Tab(tab_components, &active_tab_);
    
    // 创建顶部工具栏
    auto toolbar = createTopToolbar();
    
    // 组合所有组件
    return Container::Vertical({
        toolbar,
        tab_container
    });
}

// 创建顶部工具栏
ftxui::Component SystemInfoDialog::createTopToolbar() {
    // 创建按钮组件
    auto refresh_btn = Button("🔄 刷新", [this] { onRefresh(); });
    auto export_btn = Button("📤 导出", [this] { onExport(); });
    auto auto_refresh_btn = Button("⏰ 自动刷新", [this] { 
        auto_refresh_ = !auto_refresh_; 
        status_text_ = "自动刷新: " + std::string(auto_refresh_ ? "开启" : "关闭");
    });
    auto close_btn = Button("❌ 关闭", [this] { onClose(); });
    
    // 创建工具栏容器
    auto toolbar = Container::Horizontal({
        refresh_btn,
        export_btn,
        auto_refresh_btn,
        close_btn
    });
    
    return Renderer(toolbar, [this, refresh_btn, export_btn, auto_refresh_btn, close_btn] {
        return vbox({
            // 按钮行
            hbox({
                refresh_btn->Render() | color(Color::Cyan),
                separator(),
                export_btn->Render() | color(Color::Green),
                separator(),
                auto_refresh_btn->Render() | color(auto_refresh_ ? Color::Green : Color::Red),
                separator(),
                close_btn->Render() | color(Color::Red)
            }) | center,
            separator(),
            // 状态信息行
            hbox({
                text("📊 最后更新: " + last_update_time_) | color(Color::GrayLight),
                separator(),
                text("🔍 检测到 " + std::to_string(system_info_data_.network_info_list.size()) + " 个网络接口") | color(Color::Blue)
            }) | center
        });
    });
}


// 创建设备信息标签页
ftxui::Component SystemInfoDialog::createDeviceInfoTab() {
    return Renderer([this] {
        const auto& device_info = system_info_data_.device_info;
        Elements device_elements;
        
        device_elements.push_back(text("🖥️ 设备信息") | bold | color(Color::Blue) | center);
        device_elements.push_back(separator());
        
        // CPU 信息
        device_elements.push_back(text("💻 CPU 信息") | bold | color(Color::Cyan));
        device_elements.push_back(text("型号: " + device_info.cpu_model) | color(Color::White));
        device_elements.push_back(text("核心数: " + device_info.cpu_cores) | color(Color::White));
        device_elements.push_back(text("使用率: " + device_info.cpu_usage) | color(Color::Yellow));
        device_elements.push_back(separator());
        
        // 内存信息
        device_elements.push_back(text("🧠 内存信息") | bold | color(Color::Cyan));
        device_elements.push_back(text("总量: " + device_info.memory_total) | color(Color::White));
        device_elements.push_back(text("已用: " + device_info.memory_used) | color(Color::Red));
        device_elements.push_back(text("可用: " + device_info.memory_available) | color(Color::Green));
        device_elements.push_back(separator());
        
        // 网络设备信息
        device_elements.push_back(text("🌐 网络设备信息") | bold | color(Color::Cyan));
        device_elements.push_back(text("检测到的网络接口: " + std::to_string(system_info_data_.network_info_list.size()) + " 个") | color(Color::White));
        
        if (!system_info_data_.network_info_list.empty()) {
            device_elements.push_back(text("主要网络接口:") | color(Color::Yellow));
            for (size_t i = 0; i < std::min(system_info_data_.network_info_list.size(), size_t(3)); ++i) {
                const auto& net = system_info_data_.network_info_list[i];
                auto status_color = Color::Green;
                if (net.connection_status.find("未连接") != std::string::npos) {
                    status_color = Color::Red;
                } else if (net.connection_status.find("已连接但未运行") != std::string::npos) {
                    status_color = Color::Yellow;
                }
                
                device_elements.push_back(text("  🔌 " + net.interface_name + " - " + net.connection_status) | color(status_color));
                if (net.ip_address != "N/A") {
                    device_elements.push_back(text("    IP: " + net.ip_address) | color(Color::Green));
                }
            }
            if (system_info_data_.network_info_list.size() > 3) {
                device_elements.push_back(text("  ... 还有 " + std::to_string(system_info_data_.network_info_list.size() - 3) + " 个接口") | color(Color::GrayLight));
            }
        } else {
            device_elements.push_back(text("❌ 未检测到网络接口") | color(Color::Red));
        }
        device_elements.push_back(separator());
        
        // 系统信息
        device_elements.push_back(text("⚙️ 系统信息") | bold | color(Color::Cyan));
        device_elements.push_back(text("GPU: " + device_info.gpu_info) | color(Color::White));
        device_elements.push_back(text("操作系统: " + device_info.os_version) | color(Color::White));
        device_elements.push_back(text("内核版本: " + device_info.kernel_version) | color(Color::GrayLight));
        
        return vbox(device_elements);
    });
}

// 创建状态信息标签页
ftxui::Component SystemInfoDialog::createStatusInfoTab() {
    return Renderer([this] {
        const auto& status_info = system_info_data_.status_info;
        Elements status_elements;
        
        status_elements.push_back(text("📊 状态信息") | bold | color(Color::Blue) | center);
        status_elements.push_back(separator());
        
        // 运行状态
        status_elements.push_back(text("⏰ 运行状态") | bold | color(Color::Cyan));
        status_elements.push_back(text("运行时间: " + status_info.uptime) | color(Color::Green));
        status_elements.push_back(text("系统负载: " + status_info.load_average) | color(Color::Yellow));
        status_elements.push_back(text("进程数: " + status_info.processes) | color(Color::White));
        status_elements.push_back(text("线程数: " + status_info.threads) | color(Color::White));
        status_elements.push_back(separator());
        
        // 性能监控
        status_elements.push_back(text("📈 性能监控") | bold | color(Color::Cyan));
        status_elements.push_back(text("CPU 温度: " + status_info.cpu_temperature) | color(Color::Red));
        status_elements.push_back(text("系统负载: " + status_info.system_load) | color(Color::Yellow));
        status_elements.push_back(text("内存使用率: " + status_info.memory_usage) | color(Color::Blue));
        status_elements.push_back(text("交换区使用率: " + status_info.swap_usage) | color(Color::Magenta));
        status_elements.push_back(separator());
        
        // 网络状态
        status_elements.push_back(text("🌐 网络状态") | bold | color(Color::Cyan));
        status_elements.push_back(text("活跃网络接口: " + std::to_string(system_info_data_.network_info_list.size()) + " 个") | color(Color::White));
        
        if (!system_info_data_.network_info_list.empty()) {
            int active_count = 0;
            int inactive_count = 0;
            for (const auto& net : system_info_data_.network_info_list) {
                if (net.connection_status.find("活跃") != std::string::npos || 
                    net.connection_status.find("回环") != std::string::npos) {
                    active_count++;
                } else {
                    inactive_count++;
                }
            }
            status_elements.push_back(text("活跃接口: " + std::to_string(active_count) + " 个") | color(Color::Green));
            status_elements.push_back(text("非活跃接口: " + std::to_string(inactive_count) + " 个") | color(Color::Red));
        } else {
            status_elements.push_back(text("❌ 无网络接口") | color(Color::Red));
        }
        
        return vbox(status_elements);
    });
}

// 创建磁盘信息标签页
ftxui::Component SystemInfoDialog::createDiskInfoTab() {
    return Renderer([this] {
        Elements disk_elements;
        disk_elements.push_back(text("💾 磁盘信息") | bold | color(Color::Blue) | center);
        disk_elements.push_back(separator());
        
        // 为每个磁盘创建简洁的信息显示
        for (const auto& disk : system_info_data_.disk_info_list) {
            disk_elements.push_back(text("💽 " + disk.device_name) | bold | color(Color::Cyan));
            disk_elements.push_back(text("挂载点: " + disk.mount_point) | color(Color::GrayLight));
            disk_elements.push_back(text("文件系统: " + disk.filesystem) | color(Color::Cyan));
            disk_elements.push_back(text("总容量: " + disk.total_size) | color(Color::White));
            disk_elements.push_back(text("已使用: " + disk.used_size) | color(Color::Red));
            disk_elements.push_back(text("可用空间: " + disk.available_size) | color(Color::Green));
            disk_elements.push_back(text("使用率: " + disk.usage_percentage) | color(Color::Yellow));
            disk_elements.push_back(separator());
        }
        
        return vbox(disk_elements);
    });
}

// 创建网络信息标签页
ftxui::Component SystemInfoDialog::createNetworkInfoTab() {
    // 创建网络诊断按钮
    auto network_diagnosis_btn = Button("🔍 网络诊断", [this] {
        status_text_ = "正在执行网络诊断...";
        // 这里可以添加实际的网络诊断逻辑
    });
    
    auto refresh_network_btn = Button("🔄 刷新网络", [this] {
        system_info_data_ = info_collector_->collectAllInfo();
        status_text_ = "网络信息已刷新";
        last_update_time_ = info_collector_->getLastUpdateTime();
    });
    
    return Renderer([this, network_diagnosis_btn, refresh_network_btn] {
        Elements network_elements;
        network_elements.push_back(text("🌐 网络信息") | bold | color(Color::Blue) | center);
        network_elements.push_back(separator());
        
        // 添加网络概览信息
        network_elements.push_back(text("📊 网络概览") | bold | color(Color::Yellow));
        network_elements.push_back(text("检测到的网络接口数量: " + std::to_string(system_info_data_.network_info_list.size())) | color(Color::White));
        
        // 添加系统网络信息
        network_elements.push_back(text("🔍 系统网络状态") | bold | color(Color::Cyan));
        network_elements.push_back(text("主机名: " + system_info_data_.system_info.hostname) | color(Color::Green));
        network_elements.push_back(text("架构: " + system_info_data_.system_info.architecture) | color(Color::GrayLight));
        network_elements.push_back(separator());
        
        // 添加操作按钮
        network_elements.push_back(hbox({
            network_diagnosis_btn->Render() | color(Color::Blue),
            separator(),
            refresh_network_btn->Render() | color(Color::Green)
        }) | center);
        network_elements.push_back(separator());
        
        if (system_info_data_.network_info_list.empty()) {
            network_elements.push_back(text("❌ 未检测到网络接口") | color(Color::Red) | center);
            network_elements.push_back(text("💡 提示: 在WSL环境中，网络接口可能有限") | color(Color::Yellow) | center);
            network_elements.push_back(text("🔍 尝试运行 'ip addr' 命令查看网络接口") | color(Color::Cyan) | center);
            network_elements.push_back(text("📋 或者运行 'ifconfig' 命令查看详细信息") | color(Color::Magenta) | center);
        } else {
            // 为每个网络接口创建详细的信息显示
            for (const auto& net : system_info_data_.network_info_list) {
                // 接口名称和状态
                auto status_color = Color::Green;
                if (net.connection_status.find("未连接") != std::string::npos) {
                    status_color = Color::Red;
                } else if (net.connection_status.find("已连接但未运行") != std::string::npos) {
                    status_color = Color::Yellow;
                }
                
                network_elements.push_back(text("🔌 " + net.interface_name) | bold | color(Color::Cyan));
                
                // 基本连接信息
                network_elements.push_back(text("📡 连接状态: " + net.connection_status) | color(status_color));
                
                // IP地址信息
                if (net.ip_address != "N/A") {
                    network_elements.push_back(text("🌍 IP 地址: " + net.ip_address) | color(Color::Green));
                } else {
                    network_elements.push_back(text("🌍 IP 地址: 未分配") | color(Color::GrayLight));
                }
                
                // MAC地址信息
                if (net.mac_address != "N/A" && !net.mac_address.empty()) {
                    network_elements.push_back(text("🔗 MAC 地址: " + net.mac_address) | color(Color::GrayLight));
                }
                
                // 网络流量统计
                network_elements.push_back(text("📊 流量统计:") | bold | color(Color::Yellow));
                network_elements.push_back(text("  📥 接收: " + net.bytes_received + " (" + net.packets_received + " 包)") | color(Color::Blue));
                network_elements.push_back(text("  📤 发送: " + net.bytes_sent + " (" + net.packets_sent + " 包)") | color(Color::Red));
                
                // 计算总流量
                try {
                    // 简单的流量计算（这里可以进一步优化）
                    network_elements.push_back(text("  📈 总流量: " + net.bytes_received + " + " + net.bytes_sent) | color(Color::Magenta));
                } catch (...) {
                    network_elements.push_back(text("  📈 总流量: 计算中...") | color(Color::GrayLight));
                }
                
                network_elements.push_back(separator());
            }
        }
        
        // 添加网络诊断信息
        network_elements.push_back(text("🛠️ 网络诊断") | bold | color(Color::Red));
        network_elements.push_back(text("💡 如果网络信息不完整，可以尝试以下命令:") | color(Color::Yellow));
        network_elements.push_back(text("  • ip addr show") | color(Color::Cyan));
        network_elements.push_back(text("  • ifconfig -a") | color(Color::Cyan));
        network_elements.push_back(text("  • netstat -i") | color(Color::Cyan));
        network_elements.push_back(text("  • ss -tuln") | color(Color::Cyan));
        
        return vbox(network_elements);
    });
}

// 创建系统信息标签页
ftxui::Component SystemInfoDialog::createSystemInfoTab() {
    return Renderer([this] {
        const auto& system_info = system_info_data_.system_info;
        return vbox({
            text("⚙️ 系统信息") | bold | color(Color::Blue) | center,
            separator(),
            text("👤 用户信息") | bold | color(Color::Cyan),
            text("主机名: " + system_info.hostname) | color(Color::White),
            text("用户名: " + system_info.username) | color(Color::Green),
            text("主目录: " + system_info.home_directory) | color(Color::GrayLight),
            separator(),
            text("🌍 环境信息") | bold | color(Color::Cyan),
            text("Shell: " + system_info.shell) | color(Color::Cyan),
            text("语言: " + system_info.language) | color(Color::Yellow),
            text("时区: " + system_info.timezone) | color(Color::Magenta),
            separator(),
            text("⚙️ 系统配置") | bold | color(Color::Cyan),
            text("架构: " + system_info.architecture) | color(Color::Blue),
            text("启动时间: " + system_info.boot_time) | color(Color::Red)
        });
    });
}

// 创建设备信息面板
ftxui::Component SystemInfoDialog::createDeviceInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("设备详细信息") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建状态信息面板
ftxui::Component SystemInfoDialog::createStatusInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("系统状态监控") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建磁盘信息面板
ftxui::Component SystemInfoDialog::createDiskInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("磁盘使用情况") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建网络信息面板
ftxui::Component SystemInfoDialog::createNetworkInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("网络接口状态") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 创建系统信息面板
ftxui::Component SystemInfoDialog::createSystemInfoPanel() {
    return Renderer([this] {
        return vbox({
            text("系统配置信息") | bold | color(Color::Yellow),
            separator()
        });
    });
}

// 处理刷新按钮点击
void SystemInfoDialog::onRefresh() {
    system_info_data_ = info_collector_->collectAllInfo();
    status_text_ = "信息已刷新";
    last_update_time_ = info_collector_->getLastUpdateTime();
}

// 处理导出按钮点击
void SystemInfoDialog::onExport() {
    // 导出系统信息到文件
    std::string filename = "system_info_" + info_collector_->getLastUpdateTime() + ".txt";
    std::ofstream file(filename);
    
    if (file.is_open()) {
        file << "=== 系统信息报告 ===\n";
        file << "生成时间: " << info_collector_->getLastUpdateTime() << "\n\n";
        
        file << "=== 设备信息 ===\n";
        file << "CPU 型号: " << system_info_data_.device_info.cpu_model << "\n";
        file << "CPU 核心数: " << system_info_data_.device_info.cpu_cores << "\n";
        file << "内存总量: " << system_info_data_.device_info.memory_total << "\n";
        file << "操作系统: " << system_info_data_.device_info.os_version << "\n\n";
        
        file << "=== 状态信息 ===\n";
        file << "系统运行时间: " << system_info_data_.status_info.uptime << "\n";
        file << "系统负载: " << system_info_data_.status_info.load_average << "\n";
        file << "进程数: " << system_info_data_.status_info.processes << "\n\n";
        
        file << "=== 磁盘信息 ===\n";
        for (const auto& disk : system_info_data_.disk_info_list) {
            file << "设备: " << disk.device_name << "\n";
            file << "挂载点: " << disk.mount_point << "\n";
            file << "总容量: " << disk.total_size << "\n";
            file << "使用率: " << disk.usage_percentage << "\n\n";
        }
        
        file << "=== 网络信息 ===\n";
        for (const auto& net : system_info_data_.network_info_list) {
            file << "接口: " << net.interface_name << "\n";
            file << "IP 地址: " << net.ip_address << "\n";
            file << "连接状态: " << net.connection_status << "\n\n";
        }
        
        file.close();
        status_text_ = "信息已导出到 " + filename;
    } else {
        status_text_ = "导出失败";
    }
}

// 处理关闭按钮点击
void SystemInfoDialog::onClose() {
    dialog_open_ = false;
    if (screen_) {
        screen_->Exit();
    }
}


} // namespace UI
