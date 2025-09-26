#include "../../include/FTB/SystemInfoCollector.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace FTB {

SystemInfoCollector::SystemInfoCollector() 
    : info_cached_(false) {
    // 初始化时收集一次信息
    refreshAllInfo();
}

SystemInfoCollector::SystemInfoData SystemInfoCollector::collectAllInfo() {
    if (!info_cached_) {
        refreshAllInfo();
    }
    return cached_info_;
}

SystemInfoCollector::DeviceInfo SystemInfoCollector::collectDeviceInfo() {
    DeviceInfo info;
    
    // 获取CPU信息
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            info.cpu_model = line.substr(line.find(":") + 2);
            break;
        }
    }
    
    // 获取CPU核心数
    info.cpu_cores = std::to_string(std::thread::hardware_concurrency());
    
    // 获取内存信息
    struct sysinfo si;
    sysinfo(&si);
    info.memory_total = std::to_string(si.totalram / 1024 / 1024) + " MB";
    info.memory_used = std::to_string((si.totalram - si.freeram) / 1024 / 1024) + " MB";
    info.memory_available = std::to_string(si.freeram / 1024 / 1024) + " MB";
    
    // 获取系统信息
    struct utsname uts;
    uname(&uts);
    info.os_version = std::string(uts.sysname) + " " + std::string(uts.release);
    info.kernel_version = std::string(uts.version);
    
    // 获取GPU信息（简化版本）
    info.gpu_info = "NVIDIA/AMD/Intel GPU";
    
    // 获取CPU使用率（简化版本）
    info.cpu_usage = "N/A";
    
    return info;
}

SystemInfoCollector::StatusInfo SystemInfoCollector::collectStatusInfo() {
    StatusInfo info;
    
    // 获取系统运行时间
    struct sysinfo si;
    sysinfo(&si);
    int days = si.uptime / 86400;
    int hours = (si.uptime % 86400) / 3600;
    int minutes = (si.uptime % 3600) / 60;
    info.uptime = std::to_string(days) + "天 " + std::to_string(hours) + "小时 " + std::to_string(minutes) + "分钟";
    
    // 获取系统负载
    double load[3];
    getloadavg(load, 3);
    info.load_average = std::to_string(load[0]) + ", " + std::to_string(load[1]) + ", " + std::to_string(load[2]);
    
    // 获取进程和线程数
    info.processes = std::to_string(si.procs);
    info.threads = std::to_string(si.procs * 2); // 估算
    
    // 获取内存使用率
    double mem_usage = (double)(si.totalram - si.freeram) / si.totalram * 100;
    info.memory_usage = std::to_string(mem_usage) + "%";
    
    // 获取交换区使用率
    double swap_usage = (double)(si.totalswap - si.freeswap) / si.totalswap * 100;
    info.swap_usage = std::to_string(swap_usage) + "%";
    
    // 其他信息
    info.cpu_temperature = "N/A";
    info.system_load = std::to_string(load[0]);
    
    return info;
}

std::vector<SystemInfoCollector::DiskInfo> SystemInfoCollector::collectDiskInfo() {
    std::vector<DiskInfo> disk_list;
    
    // 读取 /proc/mounts 获取挂载信息
    std::ifstream mounts("/proc/mounts");
    std::string line;
    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string device, mount_point, filesystem;
        iss >> device >> mount_point >> filesystem;
        
        // 只处理本地文件系统
        if (filesystem != "tmpfs" && filesystem != "proc" && filesystem != "sysfs" && 
            filesystem != "devpts" && filesystem != "devtmpfs") {
            
            DiskInfo disk;
            disk.device_name = device;
            disk.mount_point = mount_point;
            disk.filesystem = filesystem;
            
            // 获取磁盘使用情况
            struct statvfs vfs;
            if (statvfs(mount_point.c_str(), &vfs) == 0) {
                unsigned long total = vfs.f_blocks * vfs.f_frsize;
                unsigned long available = vfs.f_bavail * vfs.f_frsize;
                unsigned long used = total - available;
                
                disk.total_size = formatBytes(total);
                disk.used_size = formatBytes(used);
                disk.available_size = formatBytes(available);
                disk.usage_percentage = std::to_string((double)used / total * 100) + "%";
                
                disk_list.push_back(disk);
            }
        }
    }
    
    return disk_list;
}

std::vector<SystemInfoCollector::NetworkInfo> SystemInfoCollector::collectNetworkInfo() {
    std::vector<NetworkInfo> network_list;
    
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        return network_list;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        NetworkInfo net;
        net.interface_name = ifa->ifa_name;
        net.ip_address = "N/A";
        net.mac_address = "N/A";
        net.bytes_received = "0 B";
        net.bytes_sent = "0 B";
        net.packets_received = "0";
        net.packets_sent = "0";
        net.connection_status = "未知";
        
        // 获取IP地址 (支持IPv4和IPv6)
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
            net.ip_address = ip_str;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
            net.ip_address = "[" + std::string(ip_str) + "]";
        }
        
        // 获取MAC地址
        std::ifstream mac_file("/sys/class/net/" + net.interface_name + "/address");
        if (mac_file.is_open()) {
            std::getline(mac_file, net.mac_address);
            mac_file.close();
        }
        
        // 获取网络统计信息
        std::ifstream stats_file("/proc/net/dev");
        std::string stats_line;
        while (std::getline(stats_file, stats_line)) {
            if (stats_line.find(net.interface_name + ":") != std::string::npos) {
                std::istringstream iss(stats_line);
                std::string interface;
                iss >> interface;
                interface.pop_back(); // 移除冒号
                
                unsigned long bytes_received, packets_received, bytes_sent, packets_sent;
                // 跳过错误统计字段
                iss >> bytes_received >> packets_received;
                for (int i = 0; i < 6; ++i) {
                    std::string dummy;
                    iss >> dummy;
                }
                iss >> bytes_sent >> packets_sent;
                
                net.bytes_received = formatBytes(bytes_received);
                net.bytes_sent = formatBytes(bytes_sent);
                net.packets_received = std::to_string(packets_received);
                net.packets_sent = std::to_string(packets_sent);
                break;
            }
        }
        stats_file.close();
        
        // 获取连接状态
        if (ifa->ifa_flags & IFF_UP) {
            if (ifa->ifa_flags & IFF_RUNNING) {
                if (ifa->ifa_flags & IFF_LOOPBACK) {
                    net.connection_status = "回环接口";
                } else {
                    net.connection_status = "活跃";
                }
            } else {
                net.connection_status = "已连接但未运行";
            }
        } else {
            net.connection_status = "未连接";
        }
        
        // 添加所有网络接口，包括回环接口
        // 在WSL环境中，至少应该有一个回环接口
        if (net.interface_name == "lo" || net.ip_address != "N/A" || 
            net.interface_name.find("eth") != std::string::npos ||
            net.interface_name.find("wlan") != std::string::npos ||
            net.interface_name.find("en") != std::string::npos ||
            net.interface_name.find("wl") != std::string::npos) {
            network_list.push_back(net);
        }
    }
    
    freeifaddrs(ifaddr);
    return network_list;
}

SystemInfoCollector::SystemInfo SystemInfoCollector::collectSystemInfo() {
    SystemInfo info;
    
    // 获取主机名
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    info.hostname = hostname;
    
    // 获取用户名
    info.username = getenv("USER") ? getenv("USER") : "unknown";
    
    // 获取主目录
    info.home_directory = getenv("HOME") ? getenv("HOME") : "unknown";
    
    // 获取Shell
    info.shell = getenv("SHELL") ? getenv("SHELL") : "unknown";
    
    // 获取语言
    info.language = getenv("LANG") ? getenv("LANG") : "unknown";
    
    // 获取时区
    info.timezone = getenv("TZ") ? getenv("TZ") : "unknown";
    
    // 获取架构
    struct utsname uts;
    uname(&uts);
    info.architecture = uts.machine;
    
    // 获取启动时间
    info.boot_time = "N/A";
    
    return info;
}

void SystemInfoCollector::refreshAllInfo() {
    cached_info_.device_info = collectDeviceInfo();
    cached_info_.status_info = collectStatusInfo();
    cached_info_.disk_info_list = collectDiskInfo();
    cached_info_.network_info_list = collectNetworkInfo();
    cached_info_.system_info = collectSystemInfo();
    
    last_update_time_ = getCurrentTime();
    info_cached_ = true;
}

std::string SystemInfoCollector::getLastUpdateTime() const {
    return last_update_time_;
}

std::string SystemInfoCollector::formatBytes(unsigned long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

std::string SystemInfoCollector::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace FTB
