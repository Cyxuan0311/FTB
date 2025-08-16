#include "../../include/Connection/SSHConnection.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

namespace Connection {

SSHConnection::SSHConnection()
    : session_(nullptr)
    , channel_(nullptr)
    , sock_(-1)
    , status_(SSHConnectionStatus::DISCONNECTED)
    , last_error_("")
{
    // 初始化libssh2
    if (!initializeLibSSH2()) {
        status_ = SSHConnectionStatus::ERROR;
        last_error_ = "Failed to initialize libssh2";
    }
}

SSHConnection::~SSHConnection() {
    disconnect();
    cleanup();
}

bool SSHConnection::initializeLibSSH2() {
    int rc = libssh2_init(0);
    if (rc != 0) {
        last_error_ = "Failed to initialize libssh2";
        return false;
    }
    return true;
}

void SSHConnection::cleanup() {
    if (session_) {
        libssh2_session_free(session_);
        session_ = nullptr;
    }
    libssh2_exit();
}

bool SSHConnection::validateParams(const SSHConnectionParams& params) {
    if (params.hostname.empty()) {
        last_error_ = "Hostname cannot be empty";
        return false;
    }
    
    if (params.port <= 0 || params.port > 65535) {
        last_error_ = "Invalid port number";
        return false;
    }
    
    if (params.username.empty()) {
        last_error_ = "Username cannot be empty";
        return false;
    }
    
    if (!params.use_key_auth && params.password.empty()) {
        last_error_ = "Password cannot be empty when not using key authentication";
        return false;
    }
    
    if (params.use_key_auth && params.private_key_path.empty()) {
        last_error_ = "Private key path cannot be empty when using key authentication";
        return false;
    }
    
    return true;
}

bool SSHConnection::connect(const SSHConnectionParams& params) {
    // 如果已经连接，先断开
    if (status_ == SSHConnectionStatus::CONNECTED) {
        disconnect();
    }
    
    // 验证参数
    if (!validateParams(params)) {
        return false;
    }
    
    current_params_ = params;
    status_ = SSHConnectionStatus::CONNECTING;
    
    if (status_callback_) {
        status_callback_(status_);
    }
    
    // 创建socket
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        last_error_ = "Failed to create socket";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        return false;
    }
    
    // 解析主机名
    struct hostent* host = gethostbyname(params.hostname.c_str());
    if (!host) {
        last_error_ = "Failed to resolve hostname: " + params.hostname;
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    // 设置socket地址
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(params.port);
    sin.sin_addr.s_addr = *(long*)(host->h_addr);
    
    // 连接到服务器
    if (::connect(sock_, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
        last_error_ = "Failed to connect to " + params.hostname + ":" + std::to_string(params.port);
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    // 创建SSH会话
    session_ = libssh2_session_init();
    if (!session_) {
        last_error_ = "Failed to create SSH session";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    // 设置socket
    if (libssh2_session_handshake(session_, sock_)) {
        last_error_ = "Failed to establish SSH session";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        cleanup();
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    // 认证
    int auth_result = -1;
    if (params.use_key_auth) {
        // 使用密钥认证
        auth_result = libssh2_userauth_publickey_fromfile(
            session_,
            params.username.c_str(),
            nullptr,
            params.private_key_path.c_str(),
            nullptr
        );
    } else {
        // 使用密码认证
        auth_result = libssh2_userauth_password(
            session_,
            params.username.c_str(),
            params.password.c_str()
        );
    }
    
    if (auth_result) {
        last_error_ = "Authentication failed";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) {
            status_callback_(status_);
        }
        cleanup();
        close(sock_);
        sock_ = -1;
        return false;
    }
    
    // 设置阻塞模式
    libssh2_session_set_blocking(session_, 1);
    
    status_ = SSHConnectionStatus::CONNECTED;
    if (status_callback_) {
        status_callback_(status_);
    }
    
    return true;
}

void SSHConnection::disconnect() {
    if (channel_) {
        libssh2_channel_free(channel_);
        channel_ = nullptr;
    }
    
    if (session_) {
        libssh2_session_disconnect(session_, "Normal Shutdown");
        libssh2_session_free(session_);
        session_ = nullptr;
    }
    
    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }
    
    status_ = SSHConnectionStatus::DISCONNECTED;
    if (status_callback_) {
        status_callback_(status_);
    }
}

std::string SSHConnection::executeCommand(const std::string& command) {
    if (status_ != SSHConnectionStatus::CONNECTED) {
        last_error_ = "Not connected to SSH server";
        return "";
    }
    
    // 创建通道
    channel_ = libssh2_channel_open_session(session_);
    if (!channel_) {
        last_error_ = "Failed to open SSH channel";
        return "";
    }
    
    // 执行命令
    if (libssh2_channel_exec(channel_, command.c_str())) {
        last_error_ = "Failed to execute command";
        libssh2_channel_free(channel_);
        channel_ = nullptr;
        return "";
    }
    
    // 读取输出
    std::string output;
    char buffer[1024];
    ssize_t rc;
    
    while ((rc = libssh2_channel_read(channel_, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, rc);
    }
    
    // 等待命令完成
    libssh2_channel_send_eof(channel_);
    libssh2_channel_wait_eof(channel_);
    libssh2_channel_wait_closed(channel_);
    
    // 获取退出状态
    int exit_code = libssh2_channel_get_exit_status(channel_);
    
    // 清理通道
    libssh2_channel_free(channel_);
    channel_ = nullptr;
    
    if (exit_code != 0) {
        last_error_ = "Command exited with code " + std::to_string(exit_code);
    }
    
    return output;
}

} // namespace Connection
