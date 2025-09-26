#ifndef SYSTEM_INFO_COLLECTOR_HPP
#define SYSTEM_INFO_COLLECTOR_HPP

#include <string>
#include <vector>
#include <memory>

namespace FTB {

// 系统信息收集器类
class SystemInfoCollector {
public:
    // 设备信息结构
    struct DeviceInfo {
        std::string cpu_model;
        std::string cpu_cores;
        std::string cpu_usage;
        std::string memory_total;
        std::string memory_used;
        std::string memory_available;
        std::string gpu_info;
        std::string os_version;
        std::string kernel_version;
    };
    
    // 状态信息结构
    struct StatusInfo {
        std::string uptime;
        std::string load_average;
        std::string processes;
        std::string threads;
        std::string cpu_temperature;
        std::string system_load;
        std::string memory_usage;
        std::string swap_usage;
    };
    
    // 磁盘信息结构
    struct DiskInfo {
        std::string device_name;
        std::string mount_point;
        std::string filesystem;
        std::string total_size;
        std::string used_size;
        std::string available_size;
        std::string usage_percentage;
    };
    
    // 网络信息结构
    struct NetworkInfo {
        std::string interface_name;
        std::string ip_address;
        std::string mac_address;
        std::string bytes_received;
        std::string bytes_sent;
        std::string packets_received;
        std::string packets_sent;
        std::string connection_status;
    };
    
    // 系统信息结构
    struct SystemInfo {
        std::string hostname;
        std::string username;
        std::string home_directory;
        std::string shell;
        std::string language;
        std::string timezone;
        std::string architecture;
        std::string boot_time;
    };
    
    // 完整的系统信息结构
    struct SystemInfoData {
        DeviceInfo device_info;
        StatusInfo status_info;
        std::vector<DiskInfo> disk_info_list;
        std::vector<NetworkInfo> network_info_list;
        SystemInfo system_info;
    };

    SystemInfoCollector();
    ~SystemInfoCollector() = default;

    // 获取完整的系统信息
    SystemInfoData collectAllInfo();
    
    // 获取设备信息
    DeviceInfo collectDeviceInfo();
    
    // 获取状态信息
    StatusInfo collectStatusInfo();
    
    // 获取磁盘信息
    std::vector<DiskInfo> collectDiskInfo();
    
    // 获取网络信息
    std::vector<NetworkInfo> collectNetworkInfo();
    
    // 获取系统信息
    SystemInfo collectSystemInfo();
    
    // 刷新所有信息
    void refreshAllInfo();
    
    // 获取最后更新时间
    std::string getLastUpdateTime() const;

private:
    // 辅助方法
    std::string formatBytes(unsigned long bytes);
    std::string getCurrentTime();
    
    // 私有数据成员
    SystemInfoData cached_info_;
    std::string last_update_time_;
    bool info_cached_;
};

} // namespace FTB

#endif // SYSTEM_INFO_COLLECTOR_HPP

