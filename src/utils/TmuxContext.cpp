#include "../../include/utils/TmuxContext.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <array>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

namespace FTB {

TmuxContext& TmuxContext::Instance() {
    static TmuxContext inst;
    return inst;
}

TmuxContext::TmuxContext() {
    const char* tmux_env = std::getenv("TMUX");
    if (!tmux_env || tmux_env[0] == '\0') {
        in_tmux_ = false;
        return;
    }
    in_tmux_ = true;

    // Get tmux version
    std::string raw_ver = RunTmuxCommand("tmux -V");
    tmux_version_ = ParseVersion(raw_ver);

    // Get outer terminal type via client_termname
    outer_term_ = RunTmuxCommand("tmux display-message -p '#{client_termname}'");
    TrimRight(outer_term_);

    // Try to get outer TERM_PROGRAM (for iTerm2 detection)
    // tmux update-environment may propagate this
    outer_term_program_ = RunTmuxCommand("tmux show-environment TERM_PROGRAM 2>/dev/null");
    auto eq_pos = outer_term_program_.find('=');
    if (eq_pos != std::string::npos) {
        outer_term_program_ = outer_term_program_.substr(eq_pos + 1);
    } else {
        outer_term_program_.clear();
    }
    TrimRight(outer_term_program_);

    // Check allow-passthrough
    std::string pt_output = RunTmuxCommand("tmux show-options -g allow-passthrough 2>/dev/null");
    passthrough_ = (pt_output.find("on") != std::string::npos);

    // Check terminal-features for image protocol support
    std::string tf_output = RunTmuxCommand("tmux show-options -g terminal-features 2>/dev/null");
    has_sixel_feature_ = (tf_output.find("sixel") != std::string::npos);
    has_kitty_feature_ = (tf_output.find("cpath") != std::string::npos);
    if (!has_kitty_feature_ && tf_output.find("xterm-kitty") != std::string::npos) {
        has_kitty_feature_ = true;
    }
    TrimRight(tf_output);

    // ====== DIRECT TTX AVAILABILITY ======
    outer_tty_ = RunTmuxCommand("tmux display-message -p '#{client_tty}'");
    TrimRight(outer_tty_);

    // ====== WINDOWS TERMINAL DETECTION ======
    // Check WT_SESSION directly (this is the most reliable indicator of Windows Terminal)
    const char* wt_session = std::getenv("WT_SESSION");
    if (wt_session && wt_session[0] != '\0') {
        is_windows_terminal_ = true;
    } else {
        // Also check tmux environment (may have been set before tmux started)
        std::string wt_tmux = RunTmuxCommand("tmux show-environment WT_SESSION 2>/dev/null");
        auto eq = wt_tmux.find('=');
        if (eq != std::string::npos) {
            std::string val = wt_tmux.substr(eq + 1);
            TrimRight(val);
            is_windows_terminal_ = !val.empty() && val != "-";
        } else {
            is_windows_terminal_ = false;
        }
    }

    // Get pane position offsets for cursor positioning in direct TTY write
    std::string top_str = RunTmuxCommand("tmux display-message -p '#{pane_top}'");
    TrimRight(top_str);
    pane_top_ = top_str.empty() ? 0 : std::atoi(top_str.c_str());

    std::string left_str = RunTmuxCommand("tmux display-message -p '#{pane_left}'");
    TrimRight(left_str);
    pane_left_ = left_str.empty() ? 0 : std::atoi(left_str.c_str());

    // ====== DIRECT TTY DECISION ======
    // Direct TTY write is needed when:
    // 1. In tmux with KITTY_WINDOW_ID set (outer terminal is real Kitty)
    // 2. AND NOT Windows Terminal (WT doesn't render Kitty APC)
    // 3. AND outer TTY is available
    // This bypasses tmux's ~4KB APC buffer which truncates Kitty chunks.
    const char* kitty_window = std::getenv("KITTY_WINDOW_ID");
    bool has_kitty_id = (kitty_window && kitty_window[0] != '\0');

    if (has_kitty_id && !is_windows_terminal_ && !outer_tty_.empty()) {
        needs_direct_tty_ = true;
    }
}

bool TmuxContext::HasNativeImageSupport() const {
    return in_tmux_ && VersionAtLeast(tmux_version_, "3.4");
}

bool TmuxContext::TmuxForwardsKitty() const {
    if (!in_tmux_ || !HasNativeImageSupport()) return false;
    if (outer_term_ == "xterm-kitty") return true;
    if (has_kitty_feature_) return true;
    return false;
}

bool TmuxContext::TmuxForwardsSixel() const {
    if (!in_tmux_ || !HasNativeImageSupport()) return false;
    if (has_sixel_feature_) return true;
    static const std::vector<std::string> sixel_terms = {
        "xterm-kitty", "foot", "foot-extra", "foot-direct",
        "contour", "wezterm", "xterm-direct", "xterm-24bit",
        "st-direct", "st-24bit",
    };
    for (const auto& st : sixel_terms) {
        if (outer_term_ == st) return true;
    }
    return false;
}

std::string TmuxContext::WrapPassthrough(const std::string& seq) const {
    std::string wrapped;
    wrapped.reserve(seq.size() + 16);
    wrapped.append("\033Ptmux;");
    wrapped.append(seq);
    wrapped.append("\033\\");
    return wrapped;
}

void TmuxContext::WriteWithPassthrough(int fd, const char* data, size_t len) const {
    if (!in_tmux_ || !passthrough_) {
        [[maybe_unused]] auto _ = ::write(fd, data, len);
        return;
    }
    static const char ptm_prefix[] = "\033Ptmux;";
    static const char ptm_suffix[] = "\033\\";
    [[maybe_unused]] auto _a = ::write(fd, ptm_prefix, sizeof(ptm_prefix) - 1);
    [[maybe_unused]] auto _b = ::write(fd, data, static_cast<ssize_t>(len));
    [[maybe_unused]] auto _c = ::write(fd, ptm_suffix, sizeof(ptm_suffix) - 1);
}

std::string TmuxContext::RunTmuxCommand(const std::string& cmd) {
    std::array<char, 256> buf{};
    std::string output;
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (!pipe) {
        return {};
    }
    while (auto len = std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output.append(buf.data(), std::strlen(buf.data()));
    }
    ::pclose(pipe);
    return output;
}

std::string TmuxContext::ParseVersion(const std::string& raw) {
    std::string ver;
    bool started = false;
    for (char c : raw) {
        if (std::isdigit(c)) {
            ver += c;
            started = true;
        } else if (c == '.' && started) {
            ver += c;
        } else if (started && !ver.empty()) {
            break;
        }
    }
    if (!ver.empty() && ver.back() == '.') {
        ver.pop_back();
    }
    return ver;
}

bool TmuxContext::VersionAtLeast(const std::string& have, const std::string& need) {
    auto parse_parts = [](const std::string& s) -> std::pair<int, int> {
        int major = 0, minor = 0;
        auto dot = s.find('.');
        if (dot == std::string::npos) {
            major = std::atoi(s.c_str());
        } else {
            major = std::atoi(s.substr(0, dot).c_str());
            minor = std::atoi(s.substr(dot + 1).c_str());
        }
        return {major, minor};
    };
    auto [h_maj, h_min] = parse_parts(have);
    auto [n_maj, n_min] = parse_parts(need);
    if (h_maj != n_maj) return h_maj > n_maj;
    return h_min >= n_min;
}

void TmuxContext::TrimRight(std::string& s) {
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
}

}  // namespace FTB
