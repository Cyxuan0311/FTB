#ifndef SSH_CONNECTION_HPP
#define SSH_CONNECTION_HPP

#include <string>
#include <memory>
#include <functional>
#include <libssh2.h>

namespace Connection {

// SSH连接参数结构体
struct SSHConnectionParams {
    std::string hostname;
    int port;
    std::string username;
    std::string password;
    std::string remote_directory;
    std::string private_key_path;
    bool use_key_auth;
};

// SSH连接状态枚举
enum class SSHConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// SSH连接类
class SSHConnection {
public:
    SSHConnection();
    ~SSHConnection();

    // 连接方法
    bool connect(const SSHConnectionParams& params);
    void disconnect();
    
    // 执行远程命令
    std::string executeCommand(const std::string& command);
    
    // 获取连接状态
    SSHConnectionStatus getStatus() const { return status_; }
    
    // 获取错误信息
    std::string getLastError() const { return last_error_; }
    
    // 设置状态回调
    void setStatusCallback(std::function<void(SSHConnectionStatus)> callback) {
        status_callback_ = callback;
    }

private:
    // 初始化libssh2
    bool initializeLibSSH2();
    
    // 清理资源
    void cleanup();
    
    // 验证连接参数
    bool validateParams(const SSHConnectionParams& params);

private:
    LIBSSH2_SESSION* session_;
    LIBSSH2_CHANNEL* channel_;
    int sock_;
    SSHConnectionStatus status_;
    std::string last_error_;
    std::function<void(SSHConnectionStatus)> status_callback_;
    
    // 连接参数
    SSHConnectionParams current_params_;
};

} // namespace Connection

#endif // SSH_CONNECTION_HPP
