#include "../../include/preview/MediaPreview.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ftxui/component/screen_interactive.hpp>

namespace FTB {

std::mutex MediaPreview::s_cache_mutex;
MediaCache MediaPreview::s_cache;
bool MediaPreview::s_timg_available = false;
bool MediaPreview::s_timg_checked = false;
bool MediaPreview::s_ffmpeg_available = false;
bool MediaPreview::s_ffmpeg_checked = false;
bool MediaPreview::s_enabled = true;
bool MediaPreview::s_loading = false;
pid_t MediaPreview::s_child_pid = 0;

bool MediaPreview::IsMediaFile(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return (ext == ".gif" || ext == ".mp4" || ext == ".mov" || ext == ".webm" || ext == ".avi");
}

bool MediaPreview::IsEnabled() {
    return s_enabled;
}

void MediaPreview::ToggleEnabled() {
    s_enabled = !s_enabled;
}

static std::string StripUnsupportedAnsi(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '[') {
            size_t start = i;
            i += 2;
            bool is_sgr = false;
            while (i < input.size()) {
                char c = input[i];
                if (c == 'm') {
                    is_sgr = true;
                    i++;
                    break;
                } else if (c == 'l' || c == 'h' || c == 'r' || c == 'n') {
                    i++;
                    break;
                }
                i++;
            }
            if (is_sgr) {
                output.append(input, start, i - start);
            }
        } else {
            output += input[i];
            i++;
        }
    }
    return output;
}

void MediaPreview::LoadAsync(const std::string& filePath, int panel_width) {
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.key == filePath && (s_cache.completed || s_loading)) {
            return;
        }
        s_cache.key = filePath;
        s_cache.loaded = false;
        s_cache.completed = false;
        s_cache.output.clear();
        s_loading = true;
    }

    if (!s_timg_checked) {
        s_timg_checked = true;
        FILE* fp = popen("command -v timg 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                s_timg_available = true;
            }
            pclose(fp);
        }
    }
    if (!s_timg_available) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_cache.completed = true;
        s_cache.loaded = true;
        s_loading = false;
        return;
    }

    if (!s_ffmpeg_checked) {
        s_ffmpeg_checked = true;
        FILE* fp = popen("command -v ffmpeg 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                s_ffmpeg_available = true;
            }
            pclose(fp);
        }
    }

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
            int term_h = panel_width / 2;
            int render_w = std::max(20, panel_width - 2);
            int render_h = std::max(10, term_h);
            std::string g = "-g" + std::to_string(render_w) + "x" + std::to_string(render_h);

            auto dot = filePath.find_last_of('.');
            std::string ext;
            if (dot != std::string::npos) {
                ext = filePath.substr(dot);
                for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            bool is_video = (ext == ".mp4" || ext == ".mov" || ext == ".webm" || ext == ".avi");

            std::string cmd;
            if (is_video && s_ffmpeg_available) {
                cmd = "ffmpeg -ss 5 -i \"" + filePath + "\" -vframes 1 -f image2pipe -vcodec ppm - 2>/dev/null | timg -pq " + g + " - 2>/dev/null";
            } else {
                cmd = "timg --frames=1 -pq " + g + " \"" + filePath + "\" 2>/dev/null";
            }

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
                fd = ::open("/dev/null", O_RDONLY);
                if (fd != -1) {
                    ::dup2(fd, STDIN_FILENO);
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
                if (s_cache.key != filePath) {
                    return;
                }
                if (ok && !result.empty()) {
                    std::string cleaned = StripUnsupportedAnsi(result);
                    s_cache.output = std::move(cleaned);
                }
                s_cache.completed = true;
                s_cache.loaded = true;
                s_loading = false;
            }
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(s_cache_mutex);
            if (s_cache.key == filePath) {
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

bool MediaPreview::GetCached(const std::string& path, MediaCache& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    if (s_cache.key == path && s_cache.completed) {
        cache = s_cache;
        return true;
    }
    return false;
}

void MediaPreview::PlayFullscreen(const std::string& filePath, ftxui::ScreenInteractive* screen) {
    std::string cmd = "timg \"" + filePath + "\" 2>/dev/null";
    if (screen) {
        screen->WithRestoredIO([&]() { int _ = std::system(cmd.c_str()); (void)_; })();
    } else {
        int _ = std::system(cmd.c_str());
        (void)_;
    }
}

}  // namespace FTB
