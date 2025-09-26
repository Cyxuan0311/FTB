#ifndef SYSTEM_INFO_DIALOG_HPP
#define SYSTEM_INFO_DIALOG_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include "../FTB/SystemInfoCollector.hpp"

namespace UI {

// 系统信息对话框类
class SystemInfoDialog {
public:
    SystemInfoDialog();
    ~SystemInfoDialog() = default;

    // 显示对话框
    void showDialog(ftxui::ScreenInteractive& screen);
    
    // 设置信息更新回调
    void setInfoUpdateCallback(std::function<void()> callback) {
        info_update_callback_ = callback;
    }

private:
    // 创建主对话框组件
    ftxui::Component createMainDialog();
    
    // 创建现代化标签页界面
    ftxui::Component createModernTabbedInterface();
    
    // 创建标签页组件
    ftxui::Component createDeviceInfoTab();
    ftxui::Component createStatusInfoTab();
    ftxui::Component createDiskInfoTab();
    ftxui::Component createNetworkInfoTab();
    ftxui::Component createSystemInfoTab();
    
    // 创建现代化面板组件
    ftxui::Component createLeftNavigationPanel();
    ftxui::Component createCenterWorkPanel();
    ftxui::Component createRightPropertiesPanel();
    ftxui::Component createTopToolbar();
    ftxui::Component createBottomStatusBar();
    
    // 创建设备信息组件
    ftxui::Component createDeviceInfoPanel();
    
    // 创建状态信息组件
    ftxui::Component createStatusInfoPanel();
    
    // 创建磁盘信息组件
    ftxui::Component createDiskInfoPanel();
    
    // 创建网络信息组件
    ftxui::Component createNetworkInfoPanel();
    
    // 创建系统信息组件
    ftxui::Component createSystemInfoPanel();
    
    // 处理刷新按钮点击
    void onRefresh();
    
    // 处理导出按钮点击
    void onExport();
    
    // 处理关闭按钮点击
    void onClose();

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
    
    // 系统信息收集器
    std::unique_ptr<FTB::SystemInfoCollector> info_collector_;
    
    // 缓存的系统信息
    FTB::SystemInfoCollector::SystemInfoData system_info_data_;
    
    // 按钮组件
    ftxui::Component refresh_button_;
    ftxui::Component export_button_;
    ftxui::Component close_button_;
    ftxui::Component auto_refresh_toggle_;
    
    // 回调函数
    std::function<void()> info_update_callback_;
    
    // 刷新间隔（秒）
    int refresh_interval_;
    
    // 导出格式
    std::string export_format_;
    
    // 显示选项
    bool show_detailed_info_;
    bool show_performance_metrics_;
    bool show_network_details_;
};

} // namespace UI

#endif // SYSTEM_INFO_DIALOG_HPP
