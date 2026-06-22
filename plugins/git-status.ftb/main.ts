// FTB Plugin: git-status
// Displays git branch and working tree status in the status bar
//
// Returns an array of StatusBarSegment objects:
//   { text: string, fg: string, bg: string, bold: boolean }

interface FtbContext {
    current_path: string;
    selected_file: string;
    selected_file_path: string;
    selected_is_dir: boolean;
    selected_size: number;
    selected_mime: string;
    args: Record<string, any>;
}

interface StatusBarSegment {
    text: string;
    fg: string;
    bg: string;
    bold: boolean;
}

interface FtbAPI {
    ctx: FtbContext;
    fs: {
        readFile: (path: string) => string;
        listDir: (path: string) => string[];
    };
    exec: (cmd: string, args: string[]) => string;
    log: (msg: string) => void;
}

function entry(ftb: FtbAPI): StatusBarSegment[] {
    const path = ftb.ctx.current_path;

    // Check if we're in a git repo
    let isGitRepo = false;
    try {
        const result = ftb.exec("git", ["rev-parse", "--is-inside-work-tree"]);
        isGitRepo = result.trim() === "true";
    } catch {
        return [];
    }

    if (!isGitRepo) return [];

    // Get current branch name
    let branch = "";
    try {
        branch = ftb.exec("git", ["rev-parse", "--abbrev-ref", "HEAD"]).trim();
    } catch {
        branch = "unknown";
    }

    // Get short status (porcelain format)
    let staged = 0;
    let modified = 0;
    let untracked = 0;
    let conflicted = 0;

    try {
        const statusOutput = ftb.exec("git", ["status", "--porcelain=v1"]);
        const lines = statusOutput.split("\n").filter((l: string) => l.length >= 2);

        for (const line of lines) {
            const x = line[0];  // Index status
            const y = line[1];  // Working tree status

            if (x === "?" && y === "?") {
                untracked++;
            } else if (x === "U" || y === "U" || (x === "A" && y === "A") || (x === "D" && y === "D")) {
                conflicted++;
            } else {
                if (x !== " " && x !== "?") staged++;
                if (y !== " " && y !== "?") modified++;
            }
        }
    } catch {
        // git status failed, just show branch
    }

    // Build display text
    let statusText = branch;

    // Build segments
    const segments: StatusBarSegment[] = [];

    // Branch segment (always shown)
    segments.push({
        text: " " + statusText + " ",
        fg: "#1e1e2e",
        bg: "#a6e3a1",  // Green for clean branch
        bold: true,
    });

    // Staged count (yellow)
    if (staged > 0) {
        segments.push({
            text: " +" + staged + " ",
            fg: "#1e1e2e",
            bg: "#f9e2af",
            bold: false,
        });
    }

    // Modified count (orange/peach)
    if (modified > 0) {
        segments.push({
            text: " ~" + modified + " ",
            fg: "#1e1e2e",
            bg: "#fab387",
            bold: false,
        });
    }

    // Conflicted count (red)
    if (conflicted > 0) {
        segments.push({
            text: " !" + conflicted + " ",
            fg: "#1e1e2e",
            bg: "#f38ba8",
            bold: true,
        });
    }

    // Untracked count (dim/subtle)
    if (untracked > 0) {
        segments.push({
            text: " ?" + untracked + " ",
            fg: "#cdd6f4",
            bg: "#585b70",
            bold: false,
        });
    }

    // If all clean, add a checkmark
    if (staged === 0 && modified === 0 && conflicted === 0 && untracked === 0) {
        segments[0].bg = "#a6e3a1";  // Keep green
    } else {
        // Dirty repo: change branch bg to indicate changes
        segments[0].bg = "#89b4fa";  // Blue for dirty
    }

    ftb.log("git-status: branch=" + branch + " staged=" + staged + " modified=" + modified + " untracked=" + untracked);

    return segments;
}
