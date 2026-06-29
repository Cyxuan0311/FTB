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
}

interface FtbAPI {
    ctx: FtbContext;
    exec: (cmd: string, args: string[]) => string;
    log: (msg: string) => void;
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

function detectTool(ftb: FtbAPI, tools: string[]): string | null {
    for (const t of tools) {
        if (hasTool(ftb, t)) return t;
    }
    return null;
}

function entry(ftb: FtbAPI): PreviewerDefinition {
    const file = ftb.ctx.selected_file.toLowerCase();

    if (file.endsWith(".md")) {
        const tool = detectTool(ftb, ["mdv", "glow", "mdcat", "lowdown"]);
        if (!tool) {
            ftb.log("preview-external: no markdown renderer found (try mdv)");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        return {
            command: tool,
            args: tool === "mdv" ? ["{file}"] : ["--width", "{width}", "{file}"],
            label: tool === "mdv" ? "Mdv" : tool === "glow" ? "Glow" : tool,
            ansi: true,
            timeout: 10000
        };
    }

    if (file.match(/\.(png|jpg|jpeg|gif|svg|bmp)$/)) {
        const tool = detectTool(ftb, ["chafa", "catimg", "timg", "viu"]);
        if (!tool) {
            ftb.log("preview-external: no image renderer found (try chafa)");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        if (tool === "chafa") {
            return {
                command: "chafa",
                args: ["--fill", "--symbols", "block", "--size", "{width}x80", "{file}"],
                label: "Chafa",
                ansi: true,
                timeout: 15000
            };
        }
        if (tool === "catimg") {
            return {
                command: "catimg",
                args: ["-w", "{width}", "{file}"],
                label: "CatImg",
                ansi: false,
                timeout: 15000
            };
        }
        return {
            command: tool,
            args: ["{file}"],
            label: tool,
            ansi: true,
            timeout: 10000
        };
    }

    if (file.endsWith(".pdf")) {
        const tool = detectTool(ftb, ["pdftotext", "mutool"]);
        if (!tool) {
            ftb.log("preview-external: no PDF text extractor found (try poppler-utils)");
            return { command: "", args: [], label: "", ansi: false, timeout: 5000 };
        }
        return {
            command: tool,
            args: ["-layout", "{file}", "-"],
            label: "pdftotext",
            ansi: false,
            timeout: 15000
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
