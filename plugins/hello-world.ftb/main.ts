// FTB Plugin: hello-world
// A simple example plugin that displays a greeting message

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
    ui: {
        message: (text: string) => void;
        confirm: (text: string) => boolean;
    };
    log: (msg: string) => void;
}

function entry(ftb: FtbAPI): Record<string, any> {
    const ctx = ftb.ctx;
    const fileName = ctx.selected_file || "nothing";
    const isDir = ctx.selected_is_dir;

    let msg = `Hello from FTB Plugin!`;
    msg += `\n  Current directory: ${ctx.current_path}`;
    msg += `\n  Selected: ${fileName}${isDir ? "/" : ""}`;

    if (ctx.selected_size > 0) {
        const sizeKB = (ctx.selected_size / 1024).toFixed(1);
        msg += `\n  Size: ${sizeKB} KB`;
    }

    ftb.ui.message(msg);
    ftb.log(`hello-world plugin executed for: ${fileName}`);

    return { greeting: msg, file: fileName };
}
