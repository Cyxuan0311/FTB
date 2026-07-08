// FTB Plugin: preview-external
// Use external CLI tools for file preview
//
// The entry() function must return a PreviewerDefinition object:
// {
//     command: "tool_name",
//     args: ["--flag", "{file}", "--width", "{width}"],
//     label: "Tool Name",
//     ansi: true,       // output contains ANSI escape codes
//     timeout: 10000    // max execution time in ms
// }
//
// Placeholders:
//   {file}  - absolute path to the selected file
//   {name}  - selected file name only
//   {width} - preview panel width in characters

interface FtbContext {
    current_path: string;
    selected_file: string;
    selected_file_path: string;
    selected_is_dir: boolean;
    selected_size: number;
    selected_mime: string;
    args: Record<string, any>;
    panel_width: number;
    plugin_dir: string;
}

interface FtbAPI {
    ctx: FtbContext;
    exec: (cmd: string, args: string[]) => string;
    log: (msg: string) => void;
    python: {
        call: (file: string, func: string, args: Record<string, any>) => Record<string, any>;
    };
}

interface PreviewerDefinition {
    command: string;
    args: string[];
    label: string;
    ansi: boolean;
    timeout: number;
}

function hasTool(ftb: FtbAPI, name: string): boolean {
    try {
        const r = ftb.exec("which", [name]);
        return r !== null && r !== undefined && r.length > 0 && r.trim().length > 0;
    } catch {
        return false;
    }
}

function disablePreviewToggle(ftb: FtbAPI): string {
    const flagPath = ftb.ctx.plugin_dir + "/" + ".disabled";
    try {
        const r = ftb.exec("ls", [flagPath]);
        if (r !== null && r.trim() !== "" && !r.includes("No such file")) {
            ftb.exec("rm", [flagPath]);
            return "Preview re-enabled";
        }
    } catch {}
    ftb.exec("touch", [flagPath]);
    return "Preview disabled";
}

const THEMES = ["default", "monochrome", "highcontrast", "nord", "dracula", "gruvbox"];

function readFlag(ftb: FtbAPI, name: string): string {
    const p = ftb.ctx.plugin_dir + "/" + name;
    try {
        const r = ftb.exec("cat", [p]);
        if (r !== null && r.trim() !== "" && !r.includes("No such file")) {
            return r.trim();
        }
    } catch {}
    return "";
}

function writeFlag(ftb: FtbAPI, name: string, val: string): void {
    const p = ftb.ctx.plugin_dir + "/" + name;
    ftb.exec("sh", ["-c", "echo '" + val.replace(/'/g, "'\\''") + "' > '" + p.replace(/'/g, "'\\''") + "'"]);
}

function toggleColorTheme(ftb: FtbAPI): string {
    const current = readFlag(ftb, ".color_theme");
    let idx = THEMES.indexOf(current);
    if (idx < 0 || idx >= THEMES.length - 1) idx = 0;
    else idx++;
    const next = THEMES[idx];
    if (next === "default") {
        ftb.exec("rm", [ftb.ctx.plugin_dir + "/" + ".color_theme"]);
    } else {
        writeFlag(ftb, ".color_theme", next);
    }
    return "Theme: " + next;
}

function setColorTheme(ftb: FtbAPI, name: string): string {
    if (THEMES.indexOf(name) < 0) {
        return "Unknown theme: " + name + " (try: " + THEMES.join(", ") + ")";
    }
    if (name === "default") {
        ftb.exec("rm", [ftb.ctx.plugin_dir + "/" + ".color_theme"]);
    } else {
        writeFlag(ftb, ".color_theme", name);
    }
    return "Theme: " + name;
}

function entry(ftb: FtbAPI): PreviewerDefinition {
    // ─── Plugin command dispatch ────────────────────────────────────
    const raw = ftb.ctx.args && ftb.ctx.args.command;
    if (typeof raw === "string") {
        const sp = raw.indexOf(' ');
        const cmd = sp < 0 ? raw : raw.substring(0, sp);
        const arg = sp >= 0 ? raw.substring(sp + 1).trim() : "";

        if (cmd === "disablepreview") {
            const msg = disablePreviewToggle(ftb);
            return { command: "", args: [], label: msg, ansi: false, timeout: 5000 };
        }
        if (cmd === "togglecolor") {
            const msg = toggleColorTheme(ftb);
            return { command: "", args: [], label: msg, ansi: false, timeout: 5000 };
        }
        if (cmd === "setcolor") {
            const msg = setColorTheme(ftb, arg || "default");
            return { command: "", args: [], label: msg, ansi: false, timeout: 5000 };
        }
    }

    // ─── Disabled flag check ────────────────────────────────────────
    const flagPath = ftb.ctx.plugin_dir + "/" + ".disabled";
    try {
        const r = ftb.exec("ls", [flagPath]);
        if (r !== null && r.trim() !== "" && !r.includes("No such file")) {
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
    } catch {}

    const file = ftb.ctx.selected_file.toLowerCase();

    // ─── Audio preview via Python ─────────────────────────────────────
    if (file.match(/\.(mp3|flac|ogg|wav|m4a)$/)) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview audio");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/mp3_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "Audio Info",
            ansi: true,
            timeout: 15000
        };
    }

    // ─── Font preview via Python ───────────────────────────────────────
    if (file.match(/\.(ttf|otf|woff|woff2|ttc)$/)) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview font");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/font_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "Font",
            ansi: true,
            timeout: 15000
        };
    }

    // ─── Deep Learning model preview via Python ────────────────────────
    if (file.match(/\.(pt|pth|onnx|safetensors|h5|hdf5|keras|tflite)$/)) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview DL model");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/dl_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "DL Model",
            ansi: true,
            timeout: 30000
        };
    }

    // ─── Archive preview via Python (APK, RAR, UnityPackage) ──────────
    if (file.match(/\.(apk|rar|unitypackage)$/)) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview archive");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/archive_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "Archive Info",
            ansi: true,
            timeout: 30000
        };
    }

    // ─── Software package preview via Python ───────────────────────────
    if (file.endsWith(".deb") || file.endsWith(".rpm") ||
        file.endsWith(".appimage") || file.endsWith(".snap") ||
        (file.match(/\.(zst|xz|gz)$/) && file.includes(".pkg.tar"))) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview package");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/package_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "Package",
            ansi: true,
            timeout: 15000
        };
    }

    // ─── Data file preview via Python (CSV, TSV, JSON) ────────────────
    if (file.match(/\.(csv|tsv|json)$/)) {
        if (!hasTool(ftb, "python3")) {
            ftb.log("preview-external: python3 not found, cannot preview data file");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        const scriptPath = ftb.ctx.plugin_dir + "/data_parser.py";
        return {
            command: "python3",
            args: ["-u", scriptPath, "{file}", "{width}"],
            label: "Data & Chart",
            ansi: true,
            timeout: 15000
        };
    }

    // ─── Jupyter Notebook preview via nbpreview ────────────────────────
    if (file.endsWith(".ipynb")) {
        if (!hasTool(ftb, "nbpreview")) {
            ftb.log("preview-external: nbpreview not found, cannot preview notebook");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        return {
            command: "nbpreview",
            args: ["--unicode", "-w", "{width}", "{file}"],
            label: "Notebook",
            ansi: true,
            timeout: 30000
        };
    }

    return {
        command: "file",
        args: ["-b", "{file}"],
        label: "file",
        ansi: false,
        timeout: 5000
    };
}
