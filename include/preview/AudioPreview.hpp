#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>

namespace FTB {

struct AudioCache {
    std::string key;
    std::string output;
    bool loaded = false;
    bool completed = false;
};

class AudioPreview {
public:
    static bool IsAudioFile(const std::string& filename);
    static bool IsEnabled();
    static void ToggleEnabled();
    static void LoadAsync(const std::string& filePath, int panel_width);
    static bool GetCached(const std::string& path, AudioCache& cache);

private:
    static bool s_eyed3_available;
    static bool s_eyed3_checked;
    static bool s_enabled;
    static bool s_loading;
    static pid_t s_child_pid;
    static std::mutex s_cache_mutex;
    static AudioCache s_cache;
};

}
