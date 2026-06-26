#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>

namespace FTB {

struct DocCache {
    std::string key;
    std::string output;
    bool loaded = false;
    bool completed = false;
};

class DocPreview {
public:
    static bool IsDocFile(const std::string& filename);
    static bool ShowSource();
    static void ToggleSourceMode();
    static void LoadAsync(const std::string& filePath, int panel_width);
    static bool GetCached(const std::string& path, DocCache& cache);

private:
    static bool s_pandoc_available;
    static bool s_pandoc_checked;
    static bool s_show_source;
    static bool s_loading;
    static pid_t s_child_pid;
    static std::mutex s_cache_mutex;
    static DocCache s_cache;
};

}
