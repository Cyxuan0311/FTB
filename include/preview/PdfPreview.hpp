#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>

namespace FTB {

struct PdfCache {
    std::string key;
    std::string output;
    bool loaded = false;
    bool completed = false;
};

class PdfPreview {
public:
    static bool IsPdfFile(const std::string& filename);
    static bool ShowSource();
    static void ToggleSourceMode();
    static void LoadAsync(const std::string& filePath, int panel_width);
    static bool GetCached(const std::string& path, PdfCache& cache);

private:
    static bool s_hygg_available;
    static bool s_hygg_checked;
    static bool s_show_source;
    static bool s_loading;
    static pid_t s_child_pid;
    static std::mutex s_cache_mutex;
    static PdfCache s_cache;
};

}
