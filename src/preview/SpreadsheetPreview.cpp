#include "../../include/preview/SpreadsheetPreview.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <thread>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config/ConfigManager.hpp"

namespace FTB {

std::mutex SpreadsheetPreview::s_cache_mutex;
SpreadsheetCache SpreadsheetPreview::s_cache;
bool SpreadsheetPreview::s_xleak_available = false;
bool SpreadsheetPreview::s_xleak_checked = false;
bool SpreadsheetPreview::s_show_source = false;
bool SpreadsheetPreview::s_loading = false;
pid_t SpreadsheetPreview::s_child_pid = 0;

bool SpreadsheetPreview::IsSpreadsheetFile(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".xlsx" || ext == ".xls" || ext == ".xlsm" || ext == ".xlsb" || ext == ".ods";
}

bool SpreadsheetPreview::ShowSource() {
    return s_show_source;
}

void SpreadsheetPreview::ToggleSourceMode() {
    s_show_source = !s_show_source;
}

void SpreadsheetPreview::LoadAsync(const std::string& filePath, int panel_width) {
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.key == filePath && (s_cache.completed || s_loading)) return;
        s_cache.key = filePath;
        s_cache.loaded = false;
        s_cache.completed = false;
        s_cache.output.clear();
        s_loading = true;
    }

    if (!s_xleak_checked) {
        s_xleak_checked = true;
        FILE* fp = popen("command -v xleak 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                s_xleak_available = true;
            }
            pclose(fp);
        }
    }
    if (!s_xleak_available) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_cache.completed = true;
        s_cache.loaded = true;
        s_loading = false;
        return;
    }

    // Kill any previous xleak process
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_child_pid > 0) {
            ::kill(-s_child_pid, SIGTERM);
            ::waitpid(s_child_pid, nullptr, WNOHANG);
            s_child_pid = 0;
        }
    }

    std::thread([filePath, panel_width]() {
        try {
            int max_width = std::max(20, panel_width - 6);
            auto& cfg = ConfigManager::GetInstance()->GetConfig();
            int max_rows = cfg.preview.max_spreadsheet_rows;
            std::string cmd = "xleak";
            if (max_rows > 0) {
                cmd += " --max-rows=" + std::to_string(max_rows);
            }
            cmd += " --max-width=" + std::to_string(max_width)
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

bool SpreadsheetPreview::GetCached(const std::string& path, SpreadsheetCache& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    if (s_cache.key == path && s_cache.completed) {
        cache = s_cache;
        return true;
    }
    return false;
}

}
