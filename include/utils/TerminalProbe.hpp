#pragma once

#include <string>

namespace FTB {

struct TerminalInfo {
    bool kitty = false;
    bool sixel = false;
    bool iterm2 = false;
    std::string name;    // "kitty", "wezterm", "iterm2", "windows-terminal", "foot", "unknown"
    std::string method;  // "process_tree", "env", "terminal-features", "heuristic"

    // Whether to use direct TTY write in tmux (bypass tmux's parser)
    bool needs_direct_tty = false;
};

class TerminalProbe {
public:
    static const TerminalInfo& Detect();

private:
    static TerminalInfo DoDetect();

    // Walk process tree from start_pid to find the terminal emulator
    static std::string FindTerminalByProcessTree(int start_pid);

    // Get parent PID of a process
    static int GetParentPid(int pid);

    // Get the command name of a process
    static std::string GetProcessName(int pid);

    // Map terminal name to capabilities
    static TerminalInfo NameToCapabilities(const std::string& name);

    // Fallback: use env vars and heuristics
    static TerminalInfo EnvFallback();

    // Trim trailing whitespace in-place
    static void TrimRight(std::string& s);

    // Run a shell command and capture stdout
    static std::string RunCommand(const std::string& cmd);

    static TerminalInfo s_info;
    static bool s_detected;
};

}  // namespace FTB
