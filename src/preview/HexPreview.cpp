#include "../../include/preview/HexPreview.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <thread>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/utils/PerfLogger.hpp"
#include "browser/BinaryFileHandler.hpp"
#include "config/ConfigManager.hpp"

namespace FTB {

std::mutex HexPreview::s_cache_mutex;
HexCache HexPreview::s_cache;
bool HexPreview::s_xxd_available = false;
bool HexPreview::s_xxd_checked = false;
bool HexPreview::s_enabled = true;
bool HexPreview::s_loading = false;
pid_t HexPreview::s_child_pid = 0;

bool HexPreview::IsBinaryFile(const std::string& filename) {
    return BinaryFileHandler::BinaryFileRestrictor::isBinaryFile(filename);
}

bool HexPreview::IsEnabled() {
    return s_enabled;
}

void HexPreview::ToggleEnabled() {
    s_enabled = !s_enabled;
}

void HexPreview::LoadAsync(const std::string& filePath, uintmax_t fileSize, int panel_width) {
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.key == filePath && (s_cache.completed || s_loading)) return;
        s_cache.key = filePath;
        s_cache.loaded = false;
        s_cache.completed = false;
        s_cache.output.clear();
        s_loading = true;
    }

    if (!s_xxd_checked) {
        s_xxd_checked = true;
        FILE* fp = popen("command -v xxd 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                s_xxd_available = true;
            }
            pclose(fp);
        }
    }
    if (!s_xxd_available) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_cache.completed = true;
        s_loading = false;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_child_pid > 0) {
            ::kill(-s_child_pid, SIGTERM);
            ::waitpid(s_child_pid, nullptr, WNOHANG);
            s_child_pid = 0;
        }
    }

    int cols = std::max(4, std::min(16, (panel_width - 12) / 3));
    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    uintmax_t config_limit = cfg.preview.max_hex_bytes;
    uintmax_t max_bytes = (config_limit > 0)
        ? std::min(fileSize, config_limit)
        : std::min(fileSize, static_cast<uintmax_t>(65536));

    std::thread([filePath, cols, max_bytes]() {
        PERF_LOG("preview", "HexPreview thread start: " + filePath);
        try {
            std::string cmd = "xxd -c " + std::to_string(cols)
                            + " -g 1 -l " + std::to_string(max_bytes)
                            + " \"" + filePath + "\" 2>/dev/null";

            int pipefd[2];
            if (::pipe(pipefd) == -1) {
                std::lock_guard<std::mutex> lock(s_cache_mutex);
                if (s_cache.key == filePath) {
                    s_cache.completed = true;
                    s_cache.loaded = true;
                    s_loading = false;
                }
                return;
            }

            pid_t pid = ::fork();
            if (pid == -1) {
                ::close(pipefd[0]);
                ::close(pipefd[1]);
                std::lock_guard<std::mutex> lock(s_cache_mutex);
                if (s_cache.key == filePath) {
                    s_cache.completed = true;
                    s_cache.loaded = true;
                    s_loading = false;
                }
                return;
            }

            if (pid == 0) {
                ::close(pipefd[0]);
                ::dup2(pipefd[1], STDOUT_FILENO);
                int fd = ::open("/dev/null", O_WRONLY);
                if (fd != -1) {
                    ::dup2(fd, STDERR_FILENO);
                    ::close(fd);
                }
                for (int i = 3; i < 1024; i++) ::close(i);
                ::setpgid(0, 0);
                ::execlp("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
                ::_exit(1);
            }

            ::close(pipefd[1]);
            {
                std::lock_guard<std::mutex> lock(s_cache_mutex);
                s_child_pid = pid;
            }

            std::string result;
            char buf[4096];
            ssize_t n;
            while ((n = ::read(pipefd[0], buf, sizeof(buf))) > 0) {
                result.append(buf, n);
            }
            ::close(pipefd[0]);

            int status;
            ::waitpid(pid, &status, 0);

            {
                std::lock_guard<std::mutex> lock(s_cache_mutex);
                if (s_child_pid == pid) s_child_pid = 0;
            }

            bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;

            {
                PERF_LOG("preview", "HexPreview thread done: " + filePath);
                std::lock_guard<std::mutex> lock(s_cache_mutex);
                if (s_cache.key != filePath) return;
                if (ok && !result.empty()) {
                    s_cache.output = std::move(result);
                }
                s_cache.completed = true;
                s_cache.loaded = true;
                s_loading = false;
            }
        } catch (...) {
            std::lock_guard<std::mutex> lock(s_cache_mutex);
            if (s_cache.key == filePath) {
                s_cache.completed = true;
                s_cache.loaded = true;
                s_loading = false;
            }
        }
    }).detach();
}

bool HexPreview::GetCached(const std::string& path, HexCache& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    if (s_cache.key == path && s_cache.completed) {
        cache = s_cache;
        return true;
    }
    return false;
}

} // namespace FTB
