#ifndef NETWORK_SERVICE_HPP
#define NETWORK_SERVICE_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>

namespace FTB {

// 网络连接状态枚举
enum class NetworkStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// 网络连接信息结构
struct NetworkConnectionInfo {
    std::string name;
    std::string type;           // WiFi, Ethernet, Bluetooth等
    std::string status;         // 连接状态
    std::string ip_address;     // IP地址
    std::string mac_address;    // MAC地址
    std::string gateway;        // 网关
    std::string dns;           // DNS服务器
    int signal_strength;        // 信号强度 (0-100)
    double speed_mbps;          // 连接速度 (Mbps)
    std::string last_connected; // 最后连接时间
};

// 网络统计信息结构
struct NetworkStatistics {
    uint64_t bytes_sent;        // 发送字节数
    uint64_t bytes_received;    // 接收字节数
    uint64_t packets_sent;      // 发送包数
    uint64_t packets_received;  // 接收包数
    double upload_speed;        // 上传速度 (KB/s)
    double download_speed;      // 下载速度 (KB/s)
    std::string uptime;         // 运行时间
};

// 网络服务类
class NetworkService {
public:
    NetworkService();
    ~NetworkService();

    // 获取网络连接信息
    std::vector<NetworkConnectionInfo> getConnectionInfo();
    
    // 获取网络统计信息
    NetworkStatistics getStatistics();
    
    // 测试网络连接
    bool testConnection(const std::string& host = "8.8.8.8", int timeout = 5000);
    
    // 获取网络速度
    double getNetworkSpeed();
    
    // 扫描可用网络
    std::vector<std::string> scanAvailableNetworks();
    
    // 连接网络
    bool connectToNetwork(const std::string& networkName, const std::string& password = "");
    
    // 断开网络连接
    bool disconnectNetwork();
    
    // 获取网络状态
    NetworkStatus getStatus();
    
    // 设置状态回调
    void setStatusCallback(std::function<void(NetworkStatus)> callback);
    
    // 开始监控
    void startMonitoring();
    
    // 停止监控
    void stopMonitoring();
    
    // 获取最后更新时间
    std::string getLastUpdateTime();

private:
    // 更新网络信息
    void updateNetworkInfo();
    
    // 更新统计信息
    void updateStatistics();
    
    // 监控线程
    void monitoringThread();
    
    // 解析网络配置
    void parseNetworkConfig();
    
    // 获取系统网络信息
    std::string executeSystemCommand(const std::string& command);

private:
    // 网络连接信息
    std::vector<NetworkConnectionInfo> connections_;
    NetworkStatistics statistics_;
    NetworkStatus current_status_;
    
    // 监控相关
    std::atomic<bool> monitoring_active_;
    std::thread monitoring_thread_;
    std::chrono::steady_clock::time_point last_update_;
    
    // 回调函数
    std::function<void(NetworkStatus)> status_callback_;
    
    // 配置信息
    std::string primary_interface_;
    std::string default_gateway_;
    std::vector<std::string> dns_servers_;
    
    // 统计信息缓存
    std::map<std::string, uint64_t> previous_stats_;
    std::chrono::steady_clock::time_point last_stats_update_;
};

} // namespace FTB

#endif // NETWORK_SERVICE_HPP

