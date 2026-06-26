#pragma once

#include <string>
#include <cstdio>
#include <cstdlib>

namespace FTB {

inline bool CopyToSystemClipboard(const std::string& text) {
    auto run = [&](const std::string& cmd) -> bool {
        FILE* fp = popen(cmd.c_str(), "w");
        if (!fp) return false;
        size_t written = fwrite(text.data(), 1, text.size(), fp);
        int ret = pclose(fp);
        return written == text.size() && ret == 0;
    };

    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if (wayland && wayland[0]) {
        if (run("wl-copy >/dev/null 2>&1")) return true;
    }

    const char* xdisplay = std::getenv("DISPLAY");
    if (xdisplay && xdisplay[0]) {
        if (run("xclip -selection clipboard -i >/dev/null 2>&1")) return true;
        if (run("xsel --clipboard --input >/dev/null 2>&1")) return true;
    }

#ifdef __APPLE__
    if (run("pbcopy >/dev/null 2>&1")) return true;
#endif

#ifdef _WIN32
    if (run("clip.exe >/dev/null 2>&1")) return true;
#endif

    return false;
}

} // namespace FTB
