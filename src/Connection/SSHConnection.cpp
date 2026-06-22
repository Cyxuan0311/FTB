#include "../../include/Connection/SSHConnection.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <ctime>
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
    , sftp_(nullptr)
    , sock_(-1)
    , status_(SSHConnectionStatus::DISCONNECTED)
    , last_error_("")
{
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
    if (sftp_) {
        libssh2_sftp_shutdown(sftp_);
        sftp_ = nullptr;
    }
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

bool SSHConnection::sftpInit() {
    sftp_ = libssh2_sftp_init(session_);
    if (!sftp_) {
        last_error_ = "Failed to initialize SFTP session";
        return false;
    }
    return true;
}

void SSHConnection::sftpShutdown() {
    if (sftp_) {
        libssh2_sftp_shutdown(sftp_);
        sftp_ = nullptr;
    }
}

bool SSHConnection::connect(const SSHConnectionParams& params) {
    if (status_ == SSHConnectionStatus::CONNECTED) {
        disconnect();
    }
    if (!validateParams(params)) {
        return false;
    }
    current_params_ = params;
    current_remote_path_ = params.remote_directory;
    status_ = SSHConnectionStatus::CONNECTING;
    if (status_callback_) status_callback_(status_);

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        last_error_ = "Failed to create socket";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        return false;
    }

    struct hostent* host = gethostbyname(params.hostname.c_str());
    if (!host) {
        last_error_ = "Failed to resolve hostname: " + params.hostname;
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        close(sock_); sock_ = -1;
        return false;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(params.port);
    sin.sin_addr.s_addr = *(long*)(host->h_addr);

    if (::connect(sock_, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
        last_error_ = "Failed to connect to " + params.hostname + ":" + std::to_string(params.port);
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        close(sock_); sock_ = -1;
        return false;
    }

    session_ = libssh2_session_init();
    if (!session_) {
        last_error_ = "Failed to create SSH session";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        close(sock_); sock_ = -1;
        return false;
    }

    if (libssh2_session_handshake(session_, sock_)) {
        last_error_ = "Failed to establish SSH session";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        cleanup(); close(sock_); sock_ = -1;
        return false;
    }

    int auth_result = -1;
    if (params.use_key_auth) {
        auth_result = libssh2_userauth_publickey_fromfile(
            session_, params.username.c_str(), nullptr,
            params.private_key_path.c_str(), nullptr);
    } else {
        auth_result = libssh2_userauth_password(
            session_, params.username.c_str(), params.password.c_str());
    }
    if (auth_result) {
        last_error_ = "Authentication failed";
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        cleanup(); close(sock_); sock_ = -1;
        return false;
    }

    libssh2_session_set_blocking(session_, 1);

    if (!sftpInit()) {
        status_ = SSHConnectionStatus::ERROR;
        if (status_callback_) status_callback_(status_);
        cleanup(); close(sock_); sock_ = -1;
        return false;
    }

    status_ = SSHConnectionStatus::CONNECTED;
    if (status_callback_) status_callback_(status_);
    return true;
}

void SSHConnection::disconnect() {
    if (channel_) {
        libssh2_channel_free(channel_);
        channel_ = nullptr;
    }
    sftpShutdown();
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
    if (status_callback_) status_callback_(status_);
}

std::string SSHConnection::executeCommand(const std::string& command) {
    if (status_ != SSHConnectionStatus::CONNECTED) {
        last_error_ = "Not connected to SSH server";
        return "";
    }
    channel_ = libssh2_channel_open_session(session_);
    if (!channel_) {
        last_error_ = "Failed to open SSH channel";
        return "";
    }
    if (libssh2_channel_exec(channel_, command.c_str())) {
        last_error_ = "Failed to execute command";
        libssh2_channel_free(channel_);
        channel_ = nullptr;
        return "";
    }
    std::string output;
    char buffer[1024];
    ssize_t rc;
    while ((rc = libssh2_channel_read(channel_, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, rc);
    }
    libssh2_channel_send_eof(channel_);
    libssh2_channel_wait_eof(channel_);
    libssh2_channel_wait_closed(channel_);
    int exit_code = libssh2_channel_get_exit_status(channel_);
    libssh2_channel_free(channel_);
    channel_ = nullptr;
    if (exit_code != 0) {
        last_error_ = "Command exited with code " + std::to_string(exit_code);
    }
    return output;
}

std::vector<SftpEntry> SSHConnection::listDirectory(const std::string& remote_path) {
    std::vector<SftpEntry> entries;
    std::lock_guard<std::mutex> lock(sftp_mutex_);
    if (status_ != SSHConnectionStatus::CONNECTED || !sftp_) return entries;

    LIBSSH2_SFTP_HANDLE* dir = libssh2_sftp_opendir(sftp_, remote_path.c_str());
    if (!dir) return entries;

    char buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc;
    while ((rc = libssh2_sftp_readdir_ex(dir, buffer, sizeof(buffer), nullptr, 0, &attrs)) > 0) {
        std::string name(buffer, rc);
        if (name == "." || name == "..") continue;

        SftpEntry entry;
        entry.name = name;
        entry.is_directory = false;
        entry.is_symlink = false;
        entry.file_size = 0;

        if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
            entry.is_directory = (attrs.permissions & S_IFDIR) != 0;
        }
        if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
            entry.file_size = attrs.filesize;
        }
        if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
            std::time_t t = attrs.mtime;
            std::tm* tm = std::localtime(&t);
            char time_buf[32];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm);
            entry.mod_time = time_buf;
        }
        entries.push_back(std::move(entry));
    }
    libssh2_sftp_closedir(dir);
    return entries;
}

std::string SSHConnection::readFileContent(const std::string& remote_path, size_t startLine, size_t endLine) {
    std::lock_guard<std::mutex> lock(sftp_mutex_);
    if (status_ != SSHConnectionStatus::CONNECTED || !sftp_) return "";

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(sftp_, remote_path.c_str(), 0, 0);
    if (!handle) {
        last_error_ = "Failed to open remote file: " + remote_path;
        return "";
    }

    std::string content;
    char buffer[4096];
    ssize_t rc;
    while ((rc = libssh2_sftp_read(handle, buffer, sizeof(buffer))) > 0) {
        content.append(buffer, rc);
        if (content.size() > 100 * 1024) break;
    }
    libssh2_sftp_close_handle(handle);

    std::istringstream stream(content);
    std::string line, result;
    size_t currentLine = 0;
    while (std::getline(stream, line)) {
        currentLine++;
        if (currentLine >= startLine && currentLine <= endLine) {
            result += line + "\n";
        }
        if (currentLine > endLine) break;
    }
    return result;
}

} // namespace Connection
