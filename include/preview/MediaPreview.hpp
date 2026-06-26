#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>

namespace ftxui { class ScreenInteractive; }

namespace FTB {

struct MediaCache {
    std::string key;
    std::string output;
    bool loaded = false;
    bool completed = false;
};

class MediaPreview {
public:
    static bool IsMediaFile(const std::string& filename);
    static bool IsEnabled();
    static void ToggleEnabled();
    static void LoadAsync(const std::string& filePath, int panel_width);
    static bool GetCached(const std::string& path, MediaCache& cache);
    static void PlayFullscreen(const std::string& filePath, ftxui::ScreenInteractive* screen);

private:
    static bool s_timg_available;
    static bool s_timg_checked;
    static bool s_ffmpeg_available;
    static bool s_ffmpeg_checked;
    static bool s_enabled;
    static bool s_loading;
    static pid_t s_child_pid;
    static std::mutex s_cache_mutex;
    static MediaCache s_cache;
};

}
