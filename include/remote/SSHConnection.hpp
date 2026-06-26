#ifndef SSH_CONNECTION_HPP
#define SSH_CONNECTION_HPP

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <libssh2.h>
#include <libssh2_sftp.h>

namespace Connection {

struct SftpEntry {
    std::string name;
    bool is_directory;
    bool is_symlink;
    uint64_t file_size;
    std::string mod_time;
};

struct SSHConnectionParams {
    std::string hostname;
    int port;
    std::string username;
    std::string password;
    std::string remote_directory;
    std::string private_key_path;
    bool use_key_auth;
};

enum class SSHConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

class SSHConnection {
public:
    SSHConnection();
    ~SSHConnection();

    bool connect(const SSHConnectionParams& params);
    void disconnect();

    std::string executeCommand(const std::string& command);
    std::vector<SftpEntry> listDirectory(const std::string& remote_path);
    std::string readFileContent(const std::string& remote_path, size_t startLine, size_t endLine);

    SSHConnectionStatus getStatus() const { return status_; }
    std::string getLastError() const { return last_error_; }
    bool isConnected() const { return status_ == SSHConnectionStatus::CONNECTED; }

    void setCurrentRemotePath(const std::string& path) { current_remote_path_ = path; }
    std::string getCurrentRemotePath() const { return current_remote_path_; }

    std::string getHostname() const { return current_params_.hostname; }
    std::string getUsername() const { return current_params_.username; }
    int getPort() const { return current_params_.port; }

    void setStatusCallback(std::function<void(SSHConnectionStatus)> callback) {
        status_callback_ = callback;
    }

private:
    bool initializeLibSSH2();
    void cleanup();
    bool validateParams(const SSHConnectionParams& params);
    bool sftpInit();
    void sftpShutdown();

    LIBSSH2_SESSION* session_;
    LIBSSH2_CHANNEL* channel_;
    LIBSSH2_SFTP* sftp_;
    int sock_;
    std::mutex sftp_mutex_;
    SSHConnectionStatus status_;
    std::string last_error_;
    std::string current_remote_path_;
    std::function<void(SSHConnectionStatus)> status_callback_;
    SSHConnectionParams current_params_;
};

} // namespace Connection

#endif // SSH_CONNECTION_HPP
