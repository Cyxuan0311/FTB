#include "../../include/preview/AudioPreview.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <thread>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace FTB {

std::mutex AudioPreview::s_cache_mutex;
AudioCache AudioPreview::s_cache;
bool AudioPreview::s_eyed3_available = false;
bool AudioPreview::s_eyed3_checked = false;
bool AudioPreview::s_enabled = true;
bool AudioPreview::s_loading = false;
pid_t AudioPreview::s_child_pid = 0;

bool AudioPreview::IsAudioFile(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg"
        || ext == ".m4a" || ext == ".aac" || ext == ".wma" || ext == ".aiff"
        || ext == ".alac" || ext == ".ape";
}

bool AudioPreview::IsEnabled() {
    return s_enabled;
}

void AudioPreview::ToggleEnabled() {
    s_enabled = !s_enabled;
}

void AudioPreview::LoadAsync(const std::string& filePath, int panel_width) {
    (void)panel_width;
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.key == filePath && (s_cache.completed || s_loading)) return;
        s_cache.key = filePath;
        s_cache.loaded = false;
        s_cache.completed = false;
        s_cache.output.clear();
        s_loading = true;
    }

    if (!s_eyed3_checked) {
        s_eyed3_checked = true;
        FILE* fp = popen("command -v eyeD3 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                s_eyed3_available = true;
            }
            pclose(fp);
        }
    }
    if (!s_eyed3_available) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_cache.completed = true;
        s_cache.loaded = true;
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

    std::thread([filePath]() {
        try {
            std::string cmd = "eyeD3 --no-color \"" + filePath + "\" 2>/dev/null";

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

bool AudioPreview::GetCached(const std::string& path, AudioCache& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    if (s_cache.key == path && s_cache.completed) {
        cache = s_cache;
        return true;
    }
    return false;
}

}
