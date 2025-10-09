#include "FTB/NetworkService.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include <iomanip>

namespace FTB {

NetworkService::NetworkService() 
    : current_status_(NetworkStatus::DISCONNECTED), monitoring_active_(false) {
    
    // 初始化统计信息
    statistics_ = {0, 0, 0, 0, 0.0, 0.0, "00:00:00"};
    
    // 解析网络配置
    parseNetworkConfig();
    
    // 更新网络信息
    updateNetworkInfo();
}

NetworkService::~NetworkService() {
    stopMonitoring();
}

std::vector<NetworkConnectionInfo> NetworkService::getConnectionInfo() {
    return connections_;
}

NetworkStatistics NetworkService::getStatistics() {
    return statistics_;
}

bool NetworkService::testConnection(const std::string& host, int timeout) {
    std::string command = "ping -c 1 -W " + std::to_string(timeout / 1000) + " " + host + " > /dev/null 2>&1";
    int result = std::system(command.c_str());
    return result == 0;
}

double NetworkService::getNetworkSpeed() {
    // 简单的网络速度测试实现
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (testConnection("8.8.8.8")) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // 简单的速度估算（实际实现需要更复杂的算法）
        double latency = duration.count();
        if (latency > 0) {
            return 1000.0 / latency; // 简化的速度计算
        }
    }
    return 0.0;
}

std::vector<std::string> NetworkService::scanAvailableNetworks() {
    std::vector<std::string> networks;
    
    // 使用nmcli扫描WiFi网络
    std::string command = "nmcli -t -f SSID dev wifi list 2>/dev/null";
    std::string result = executeSystemCommand(command);
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line != "--") {
                networks.push_back(line);
            }
        }
    }
    
    return networks;
}

bool NetworkService::connectToNetwork(const std::string& networkName, const std::string& password) {
    std::string command;
    if (password.empty()) {
        command = "nmcli dev wifi connect \"" + networkName + "\"";
    } else {
        command = "nmcli dev wifi connect \"" + networkName + "\" password \"" + password + "\"";
    }
    
    int result = std::system(command.c_str());
    return result == 0;
}

bool NetworkService::disconnectNetwork() {
    std::string command = "nmcli dev disconnect";
    int result = std::system(command.c_str());
    return result == 0;
}

NetworkStatus NetworkService::getStatus() {
    return current_status_;
}

void NetworkService::setStatusCallback(std::function<void(NetworkStatus)> callback) {
    status_callback_ = callback;
}

void NetworkService::startMonitoring() {
    if (!monitoring_active_) {
        monitoring_active_ = true;
        monitoring_thread_ = std::thread(&NetworkService::monitoringThread, this);
    }
}

void NetworkService::stopMonitoring() {
    if (monitoring_active_) {
        monitoring_active_ = false;
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
    }
}

std::string NetworkService::getLastUpdateTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void NetworkService::updateNetworkInfo() {
    connections_.clear();
    
    // 获取网络接口信息
    std::string command = "ip addr show";
    std::string result = executeSystemCommand(command);
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string line;
        NetworkConnectionInfo current_connection;
        bool in_interface = false;
        
        while (std::getline(iss, line)) {
            // 检测网络接口
            if (line.find(": ") != std::string::npos && line.find(": ") < 10) {
                if (in_interface && !current_connection.name.empty()) {
                    connections_.push_back(current_connection);
                }
                
                // 解析接口名称
                size_t start = line.find(": ") + 2;
                size_t end = line.find(":", start);
                if (end != std::string::npos) {
                    current_connection.name = line.substr(start, end - start);
                    current_connection.type = "Ethernet";
                    current_connection.status = "Unknown";
                    current_connection.signal_strength = 0;
                    current_connection.speed_mbps = 0.0;
                    in_interface = true;
                }
            }
            
            // 检测IP地址
            if (in_interface && line.find("inet ") != std::string::npos) {
                std::regex ip_regex(R"(inet (\d+\.\d+\.\d+\.\d+))");
                std::smatch match;
                if (std::regex_search(line, match, ip_regex)) {
                    current_connection.ip_address = match[1].str();
                    current_connection.status = "Connected";
                }
            }
            
            // 检测MAC地址
            if (in_interface && line.find("link/ether ") != std::string::npos) {
                std::regex mac_regex(R"(link/ether ([0-9a-fA-F:]{17}))");
                std::smatch match;
                if (std::regex_search(line, match, mac_regex)) {
                    current_connection.mac_address = match[1].str();
                }
            }
        }
        
        // 添加最后一个接口
        if (in_interface && !current_connection.name.empty()) {
            connections_.push_back(current_connection);
        }
    }
    
    // 获取WiFi信息
    command = "nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list";
    result = executeSystemCommand(command);
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find(":") != std::string::npos) {
                std::vector<std::string> parts;
                std::istringstream line_stream(line);
                std::string part;
                while (std::getline(line_stream, part, ':')) {
                    parts.push_back(part);
                }
                
                if (parts.size() >= 3) {
                    NetworkConnectionInfo wifi_connection;
                    wifi_connection.name = parts[0];
                    wifi_connection.type = "WiFi";
                    wifi_connection.status = "Available";
                    wifi_connection.signal_strength = std::stoi(parts[1]);
                    wifi_connection.speed_mbps = 0.0; // 需要额外获取
                    connections_.push_back(wifi_connection);
                }
            }
        }
    }
    
    last_update_ = std::chrono::steady_clock::now();
}

void NetworkService::updateStatistics() {
    // 获取网络统计信息
    std::string command = "cat /proc/net/dev";
    std::string result = executeSystemCommand(command);
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.find(":") != std::string::npos && line.find("eth") != std::string::npos) {
                std::vector<std::string> parts;
                std::istringstream line_stream(line);
                std::string part;
                while (std::getline(line_stream, part, ' ')) {
                    if (!part.empty()) {
                        parts.push_back(part);
                    }
                }
                
                if (parts.size() >= 10) {
                    try {
                        uint64_t bytes_received = std::stoull(parts[1]);
                        uint64_t packets_received = std::stoull(parts[2]);
                        uint64_t bytes_sent = std::stoull(parts[9]);
                        uint64_t packets_sent = std::stoull(parts[10]);
                        
                        statistics_.bytes_received += bytes_received;
                        statistics_.packets_received += packets_received;
                        statistics_.bytes_sent += bytes_sent;
                        statistics_.packets_sent += packets_sent;
                    } catch (const std::exception& e) {
                        // 忽略解析错误
                    }
                }
            }
        }
    }
    
    last_stats_update_ = std::chrono::steady_clock::now();
}

void NetworkService::monitoringThread() {
    while (monitoring_active_) {
        updateNetworkInfo();
        updateStatistics();
        
        // 更新状态
        if (connections_.empty()) {
            current_status_ = NetworkStatus::DISCONNECTED;
        } else {
            bool has_connected = false;
            for (const auto& conn : connections_) {
                if (conn.status == "Connected") {
                    has_connected = true;
                    break;
                }
            }
            current_status_ = has_connected ? NetworkStatus::CONNECTED : NetworkStatus::DISCONNECTED;
        }
        
        // 调用状态回调
        if (status_callback_) {
            status_callback_(current_status_);
        }
        
        // 等待5秒后再次更新
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void NetworkService::parseNetworkConfig() {
    // 获取默认网关
    std::string command = "ip route | grep default | head -1 | awk '{print $3}'";
    default_gateway_ = executeSystemCommand(command);
    
    // 获取DNS服务器
    command = "cat /etc/resolv.conf | grep nameserver | awk '{print $2}'";
    std::string dns_result = executeSystemCommand(command);
    if (!dns_result.empty()) {
        std::istringstream iss(dns_result);
        std::string dns;
        while (std::getline(iss, dns)) {
            if (!dns.empty()) {
                dns_servers_.push_back(dns);
            }
        }
    }
}

std::string NetworkService::executeSystemCommand(const std::string& command) {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
    }
    return result;
}

} // namespace FTB
