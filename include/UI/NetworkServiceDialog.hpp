#ifndef NETWORK_SERVICE_DIALOG_HPP
#define NETWORK_SERVICE_DIALOG_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include "../FTB/NetworkService.hpp"

namespace UI {

// 网络服务对话框类
class NetworkServiceDialog {
public:
    NetworkServiceDialog();
    ~NetworkServiceDialog() = default;

    // 显示对话框
    void showDialog(ftxui::ScreenInteractive& screen);
    
    // 设置状态更新回调
    void setStatusUpdateCallback(std::function<void()> callback) {
        status_update_callback_ = callback;
    }

private:
    // 创建主对话框组件
    ftxui::Component createMainDialog();
    
    // 创建现代化标签页界面
    ftxui::Component createModernTabbedInterface();
    
    // 创建标签页组件
    ftxui::Component createConnectionTab();
    ftxui::Component createStatisticsTab();
    ftxui::Component createSpeedTestTab();
    ftxui::Component createSettingsTab();
    
    // 创建连接信息组件
    ftxui::Component createConnectionInfoPanel();
    
    // 创建统计信息组件
    ftxui::Component createStatisticsPanel();
    
    // 创建速度测试组件
    ftxui::Component createSpeedTestPanel();
    
    // 创建设置组件
    ftxui::Component createSettingsPanel();
    
    // 处理刷新按钮点击
    void onRefresh();
    
    // 处理连接按钮点击
    void onConnect();
    
    // 处理断开按钮点击
    void onDisconnect();
    
    // 处理关闭按钮点击
    void onClose();
    
    // 处理速度测试
    void onSpeedTest();

private:
    // 状态文本
    std::string status_text_;
    std::string last_update_time_;
    
    // 对话框状态
    bool dialog_open_;
    bool auto_refresh_;
    
    // 屏幕引用
    ftxui::ScreenInteractive* screen_;
    
    // 当前活动标签页
    int active_tab_;
    
    // 网络服务
    std::unique_ptr<FTB::NetworkService> network_service_;
    
    // 缓存的网络信息
    std::vector<FTB::NetworkConnectionInfo> connections_;
    FTB::NetworkStatistics statistics_;
    
    // 按钮组件
    ftxui::Component refresh_button_;
    ftxui::Component connect_button_;
    ftxui::Component disconnect_button_;
    ftxui::Component close_button_;
    ftxui::Component speed_test_button_;
    ftxui::Component auto_refresh_toggle_;
    
    // 回调函数
    std::function<void()> status_update_callback_;
    
    // 刷新间隔（秒）
    int refresh_interval_;
    
    // 显示选项
    bool show_detailed_info_;
    bool show_performance_metrics_;
    bool show_network_details_;
    
    // 速度测试结果
    double last_speed_test_result_;
    std::string speed_test_status_;
};

} // namespace UI

#endif // NETWORK_SERVICE_DIALOG_HPP

