#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>

namespace FTB {

struct HexCache {
    std::string key;
    std::string output;
    bool loaded = false;
    bool completed = false;
};

class HexPreview {
public:
    static bool IsBinaryFile(const std::string& filename);
    static bool IsEnabled();
    static void ToggleEnabled();
    static void LoadAsync(const std::string& filePath, uintmax_t fileSize, int panel_width);
    static bool GetCached(const std::string& path, HexCache& cache);

private:
    static bool s_xxd_available;
    static bool s_xxd_checked;
    static bool s_enabled;
    static bool s_loading;
    static pid_t s_child_pid;
    static std::mutex s_cache_mutex;
    static HexCache s_cache;
};

} // namespace FTB
