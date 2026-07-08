#include "../../include/utils/TerminalProbe.hpp"
#include "../../include/utils/TmuxContext.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>

namespace FTB {

TerminalInfo TerminalProbe::s_info;
bool TerminalProbe::s_detected = false;

const TerminalInfo& TerminalProbe::Detect() {
    if (s_detected) return s_info;
    s_info = DoDetect();
    s_detected = true;
    return s_info;
}

TerminalInfo TerminalProbe::DoDetect() {
    // Step 1: Process tree detection (most reliable)
    auto& tmux = TmuxContext::Instance();
    int start_pid = 0;

    if (tmux.InTmux()) {
        // In tmux: walk from the tmux client PID
        // The client process is the one connected to the outer terminal
        std::string client_pid_str = RunCommand(
            "tmux display-message -p '#{client_pid}' 2>/dev/null");
        TrimRight(client_pid_str);
        start_pid = std::atoi(client_pid_str.c_str());
    }

    if (start_pid <= 0) {
        // Not in tmux or failed to get client PID
        // Walk from our own parent
        start_pid = ::getppid();
    }

    std::string term_name = FindTerminalByProcessTree(start_pid);

    if (!term_name.empty() && term_name != "unknown") {
        TerminalInfo info = NameToCapabilities(term_name);
        info.method = "process_tree";

        // In tmux, if we detected a specific terminal, we need direct TTY write
        // to bypass tmux's parser (which may truncate large DCS/APC sequences)
        if (tmux.InTmux() && !tmux.OuterTty().empty()) {
            info.needs_direct_tty = true;
        }

        return info;
    }

    // Step 2: Env var / heuristic fallback
    TerminalInfo info = EnvFallback();

    return info;
}

std::string TerminalProbe::FindTerminalByProcessTree(int start_pid) {
    int pid = start_pid;
    int max_depth = 30;

    while (pid > 1 && max_depth-- > 0) {
        std::string name = GetProcessName(pid);
        if (name.empty()) {
            break;
        }

        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Known terminal emulators
        if (lower.find("kitty") != std::string::npos) return "kitty";
        if (lower.find("wezterm") != std::string::npos) return "wezterm";
        if (lower.find("iterm") != std::string::npos) return "iterm2";
        if (lower == "foot" || lower.find("foot-client") != std::string::npos) return "foot";
        if (lower.find("alacritty") != std::string::npos) return "alacritty";
        if (lower.find("contour") != std::string::npos) return "contour";
        if (lower == "xterm" || lower.find("xterm-") == 0) return "xterm";
        if (lower.find("gnome-terminal") != std::string::npos) return "gnome-terminal";
        if (lower.find("konsole") != std::string::npos) return "konsole";
        if (lower.find("mintty") != std::string::npos) return "mintty";
        if (lower.find("rxvt") != std::string::npos) return "rxvt";
        if (lower == "st" || lower.find("st-term") != std::string::npos) return "st";
        if (lower.find("tilix") != std::string::npos) return "tilix";
        if (lower.find("ghostty") != std::string::npos) return "ghostty";
        if (lower.find("rio") != std::string::npos) return "rio";
        if (lower.find("blackbox") != std::string::npos) return "blackbox";
        if (lower.find("lagrange") != std::string::npos) return "lagrange";
        if (lower.find("yakuake") != std::string::npos) return "yakuake";
        if (lower.find("terminology") != std::string::npos) return "terminology";
        if (lower.find("sakura") != std::string::npos) return "sakura";
        if (lower.find("lxterminal") != std::string::npos) return "lxterminal";
        if (lower.find("mate-terminal") != std::string::npos) return "mate-terminal";
        if (lower.find("qterminal") != std::string::npos) return "qterminal";
        if (lower.find("guake") != std::string::npos) return "guake";
        if (lower.find("kmscon") != std::string::npos) return "kmscon";

        // Skip common non-terminal processes
        if (lower.find("tmux") != std::string::npos ||
            lower.find("bash") != std::string::npos ||
            lower.find("zsh") != std::string::npos ||
            lower.find("fish") != std::string::npos ||
            lower.find("dash") != std::string::npos ||
            lower == "sh" ||
            lower.find("sshd") != std::string::npos ||
            lower.find("login") != std::string::npos ||
            lower.find("sudo") != std::string::npos ||
            lower.find("screen") != std::string::npos ||
            lower.find("nu") == 0 ||
            lower.find("byobu") != std::string::npos ||
            lower.find("mosh") != std::string::npos ||
            lower.find("systemd") != std::string::npos ||
            lower.find("cron") != std::string::npos ||
            lower.find("at-spi") != std::string::npos ||
            lower.find("dbus") != std::string::npos) {
            pid = GetParentPid(pid);
            continue;
        }

        // Unknown process — skip and continue up
        pid = GetParentPid(pid);
    }

    return "unknown";
}

int TerminalProbe::GetParentPid(int pid) {
    // Try /proc/[pid]/status (Linux)
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    FILE* f = std::fopen(path.c_str(), "r");
    if (f) {
        char buf[256];
        while (std::fgets(buf, sizeof(buf), f)) {
            if (std::strncmp(buf, "PPid:", 5) == 0) {
                std::fclose(f);
                return std::atoi(buf + 5);
            }
        }
        std::fclose(f);
    }

    // Fallback: use ps (macOS and other Unix)
    char cmd[64];
    std::snprintf(cmd, sizeof(cmd), "ps -o ppid= -p %d 2>/dev/null", pid);
    FILE* pipe = ::popen(cmd, "r");
    if (pipe) {
        char buf[64];
        if (std::fgets(buf, sizeof(buf), pipe)) {
            ::pclose(pipe);
            return std::atoi(buf);
        }
        ::pclose(pipe);
    }

    return 0;
}

std::string TerminalProbe::GetProcessName(int pid) {
    // Try /proc/[pid]/comm (Linux — process command name)
    std::string path = "/proc/" + std::to_string(pid) + "/comm";
    FILE* f = std::fopen(path.c_str(), "r");
    if (f) {
        char buf[256];
        if (std::fgets(buf, sizeof(buf), f)) {
            std::fclose(f);
            size_t len = std::strlen(buf);
            while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
                buf[--len] = '\0';
            return std::string(buf);
        }
        std::fclose(f);
    }

    // Try /proc/[pid]/exe (Linux — symlink to executable)
    path = "/proc/" + std::to_string(pid) + "/exe";
    char buf[256];
    ssize_t len = ::readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        std::string exe(buf);
        size_t pos = exe.rfind('/');
        if (pos != std::string::npos) return exe.substr(pos + 1);
        return exe;
    }

    // Fallback: ps (macOS and other Unix)
    char cmd[64];
    std::snprintf(cmd, sizeof(cmd), "ps -o comm= -p %d 2>/dev/null", pid);
    FILE* pipe = ::popen(cmd, "r");
    if (pipe) {
        char buf2[256];
        if (std::fgets(buf2, sizeof(buf2), pipe)) {
            ::pclose(pipe);
            size_t len2 = std::strlen(buf2);
            while (len2 > 0 && (buf2[len2 - 1] == '\n' || buf2[len2 - 1] == '\r'))
                buf2[--len2] = '\0';
            return std::string(buf2);
        }
        ::pclose(pipe);
    }

    return "";
}

TerminalInfo TerminalProbe::NameToCapabilities(const std::string& name) {
    TerminalInfo info;
    info.name = name;

    if (name == "kitty") {
        info.kitty = true;
        info.sixel = true;
    } else if (name == "wezterm") {
        info.sixel = true;
        info.iterm2 = true;
    } else if (name == "iterm2") {
        info.iterm2 = true;
    } else if (name == "foot") {
        info.sixel = true;
    } else if (name == "contour") {
        info.sixel = true;
    } else if (name == "xterm") {
        info.sixel = true;
    }
    // Other terminals: no known image protocol by default

    return info;
}

TerminalInfo TerminalProbe::EnvFallback() {
    TerminalInfo info;

    auto& tmux = TmuxContext::Instance();

    // Check terminal-features (user-configured, reliable if set)
    if (tmux.InTmux()) {
        if (tmux.HasKittyFeature() && tmux.HasNativeImageSupport()) {
            info.name = "kitty";
            info.kitty = true;
            info.sixel = true;
            info.method = "terminal-features";
            if (!tmux.OuterTty().empty()) info.needs_direct_tty = true;
            return info;
        }
        if (tmux.HasSixelFeature() && tmux.HasNativeImageSupport()) {
            info.name = "sixel-capable";
            info.sixel = true;
            info.method = "terminal-features";
            if (!tmux.OuterTty().empty()) info.needs_direct_tty = true;
            return info;
        }
    }

    // Check WT_SESSION → Windows Terminal (common in WSL)
    // Only use this if process tree didn't find a known terminal
    const char* wt_session = std::getenv("WT_SESSION");
    if (wt_session && wt_session[0] != '\0') {
        info.name = "windows-terminal";
        info.sixel = true;
        info.method = "env";
        if (tmux.InTmux() && !tmux.OuterTty().empty()) info.needs_direct_tty = true;
        return info;
    }

    // Check KITTY_WINDOW_ID
    const char* kitty_window = std::getenv("KITTY_WINDOW_ID");
    if (kitty_window && kitty_window[0] != '\0') {
        info.name = "kitty";
        info.kitty = true;
        info.sixel = true;
        info.method = "env";
        if (tmux.InTmux() && !tmux.OuterTty().empty()) info.needs_direct_tty = true;
        return info;
    }

    // Check TERM_PROGRAM
    const char* term_program = std::getenv("TERM_PROGRAM");
    if (term_program) {
        std::string tp(term_program);
        if (tp == "iTerm.app") {
            info.name = "iterm2";
            info.iterm2 = true;
            info.method = "env";
            if (tmux.InTmux() && tmux.PassthroughEnabled()) {
                // iTerm2 uses OSC with BEL, safe for passthrough
            }
            return info;
        }
        if (tp.find("WezTerm") != std::string::npos) {
            info.name = "wezterm";
            info.sixel = true;
            info.iterm2 = true;
            info.method = "env";
            if (tmux.InTmux() && !tmux.OuterTty().empty()) info.needs_direct_tty = true;
            return info;
        }
    }

    // Check TERM
    const char* term = std::getenv("TERM");
    if (term) {
        std::string t(term);
        if (t == "xterm-kitty") {
            info.name = "kitty";
            info.kitty = true;
            info.sixel = true;
            info.method = "env";
            if (tmux.InTmux() && !tmux.OuterTty().empty()) info.needs_direct_tty = true;
            return info;
        }
    }

    // Check tmux outer TERM
    if (tmux.InTmux() && tmux.OuterTermName() == "xterm-kitty") {
        info.name = "kitty";
        info.kitty = true;
        info.sixel = true;
        info.method = "env";
        if (!tmux.OuterTty().empty()) info.needs_direct_tty = true;
        return info;
    }

    // Check outer TERM against Sixel whitelist
    if (tmux.InTmux()) {
        static const std::vector<std::string> sixel_terms = {
            "foot", "foot-extra", "foot-direct",
            "contour", "wezterm", "xterm-direct", "xterm-24bit",
            "st-direct", "st-24bit",
        };
        for (const auto& st : sixel_terms) {
            if (tmux.OuterTermName() == st) {
                info.name = "sixel-capable";
                info.sixel = true;
                info.method = "env";
                if (!tmux.OuterTty().empty()) info.needs_direct_tty = true;
                return info;
            }
        }
    }

    info.name = "unknown";
    info.method = "heuristic";
    return info;
}

void TerminalProbe::TrimRight(std::string& s) {
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
}

std::string TerminalProbe::RunCommand(const std::string& cmd) {
    std::array<char, 256> buf{};
    std::string output;
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (!pipe) return {};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output.append(buf.data(), std::strlen(buf.data()));
    }
    ::pclose(pipe);
    return output;
}

}  // namespace FTB
