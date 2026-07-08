#pragma once

#include <string>

namespace FTB {

class TmuxContext {
public:
    static TmuxContext& Instance();

    bool InTmux() const { return in_tmux_; }
    bool PassthroughEnabled() const { return passthrough_; }
    const std::string& OuterTermName() const { return outer_term_; }
    const std::string& OuterTermProgram() const { return outer_term_program_; }
    const std::string& TmuxVersion() const { return tmux_version_; }
    const std::string& OuterTty() const { return outer_tty_; }
    int PaneTop() const { return pane_top_; }
    int PaneLeft() const { return pane_left_; }

    // Returns true if tmux version >= 3.4 (has native image protocol support)
    bool HasNativeImageSupport() const;

    // Returns true if terminal-features includes 'sixel' feature for the outer terminal
    bool HasSixelFeature() const { return has_sixel_feature_; }

    // Returns true if terminal-features includes 'cpath' (Kitty CSI u) for the outer terminal
    bool HasKittyFeature() const { return has_kitty_feature_; }

    // Returns true if tmux will natively forward Kitty APC sequences
    // (outer TERM=xterm-kitty or terminal-features has kitty, and tmux >= 3.4)
    bool TmuxForwardsKitty() const;

    // Returns true if tmux will natively forward Sixel DCS sequences
    // (outer TERM in sixel whitelist or terminal-features has sixel, and tmux >= 3.4)
    bool TmuxForwardsSixel() const;

    // Returns true when we need direct TTY write (in tmux, outer terminal
    // supports the protocol, but tmux won't forward the sequences)
    bool NeedsDirectTty() const { return needs_direct_tty_; }

    // Returns true if the outer terminal appears to be Windows Terminal
    // (detected via WT_SESSION env var in tmux environment)
    bool IsWindowsTerminal() const { return is_windows_terminal_; }

    // Wrap an escape sequence in tmux passthrough format:
    //   \ePtmux;<seq>\e\\ 
    // Only call when InTmux() && PassthroughEnabled()
    std::string WrapPassthrough(const std::string& seq) const;

    // Write data to fd with passthrough wrapping if needed
    void WriteWithPassthrough(int fd, const char* data, size_t len) const;

private:
    TmuxContext();

    bool in_tmux_ = false;
    bool passthrough_ = false;
    bool has_sixel_feature_ = false;
    bool has_kitty_feature_ = false;
    bool needs_direct_tty_ = false;
    bool is_windows_terminal_ = false;
    std::string outer_term_;
    std::string outer_term_program_;
    std::string tmux_version_;
    std::string outer_tty_;
    int pane_top_ = 0;
    int pane_left_ = 0;

    // Helper: run a tmux command and capture stdout
    static std::string RunTmuxCommand(const std::string& cmd);
    // Helper: parse tmux version string (e.g. "tmux 3.4a" → "3.4")
    static std::string ParseVersion(const std::string& raw);
    // Helper: compare version strings (e.g. "3.4" >= "3.4")
    static bool VersionAtLeast(const std::string& have, const std::string& need);
    // Helper: trim trailing whitespace
    static void TrimRight(std::string& s);
};

}  // namespace FTB
