// FTB Plugin: file-info
// Display detailed file information with human-readable size formatting

interface FtbContext {
    current_path: string;
    selected_file: string;
    selected_file_path: string;
    selected_is_dir: boolean;
    selected_size: number;
    selected_mime: string;
    args: Record<string, any>;
}

interface FtbAPI {
    ctx: FtbContext;
    fs: {
        readFile: (path: string) => string;
        listDir: (path: string) => string[];
    };
    ui: {
        message: (text: string) => void;
    };
    log: (msg: string) => void;
}

function formatSize(bytes: number): string {
    if (bytes === 0) return "0 B";
    const units = ["B", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return (bytes / Math.pow(1024, i)).toFixed(1) + " " + units[i];
}

function entry(ftb: FtbAPI): Record<string, any> {
    const ctx = ftb.ctx;

    if (!ctx.selected_file) {
        ftb.ui.message("No file selected");
        return { error: "no file selected" };
    }

    const lines: string[] = [];
    lines.push(`File: ${ctx.selected_file}`);
    lines.push(`Path: ${ctx.selected_file_path}`);
    lines.push(`Type: ${ctx.selected_is_dir ? "Directory" : "File"}`);

    if (!ctx.selected_is_dir && ctx.selected_size > 0) {
        lines.push(`Size: ${formatSize(ctx.selected_size)} (${ctx.selected_size} bytes)`);
    }

    if (ctx.selected_mime) {
        lines.push(`MIME: ${ctx.selected_mime}`);
    }

    // If directory, list contents count
    if (ctx.selected_is_dir) {
        try {
            const contents = ftb.fs.listDir(ctx.selected_file_path);
            lines.push(`Items: ${contents.length} entries`);
        } catch (e) {
            lines.push("Items: (unable to read)");
        }
    }

    const message = lines.join("\n");
    ftb.ui.message(message);
    ftb.log(`file-info: displayed info for ${ctx.selected_file}`);

    return { file: ctx.selected_file, size: ctx.selected_size };
}
