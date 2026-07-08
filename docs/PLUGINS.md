[中文文档](PLUGINS_CN.md)  |  English

# FTB Plugin System

FTB supports a TypeScript/JavaScript plugin system inspired by [yazi](https://yazi-rs.github.io/docs/plugins/overview), allowing you to extend functionality with custom scripts.

## Quick Start

### Install a Plugin

Place plugin directories in `~/.config/ftb/plugins/`:

```
~/.config/ftb/plugins/
├── hello-world.ftb/
│   ├── main.ts          # Entry point
│   └── package.json     # Metadata & permissions
└── file-info.ftb/
    ├── main.ts
    └── package.json
```

### Open Plugin Manager

Press `Ctrl+B` then type `:plugin` or `:pl`.

### Run a Plugin

In the plugin manager panel, select a plugin and press `Enter`.

## Plugin Types

| Type | Description | Entry Function | Return |
|------|-------------|----------------|--------|
| `functional` | Execute an action | `entry(ftb)` | `Record<string, any>` |
| `previewer` | Custom file preview | `entry(ftb)` | `Record<string, any>` |
| `fetcher` | Async metadata fetch | `entry(ftb)` | `Record<string, any>` |
| `statusbar` | Status bar display | `entry(ftb)` | `StatusBarSegment[]` |

### StatusBar Plugin

StatusBar plugins run automatically every 2 seconds and contribute segments to the status bar. The entry function must return an array of segment objects:

```typescript
interface StatusBarSegment {
    text: string;       // Display text
    fg: string;         // Foreground color (hex, e.g. "#cdd6f4")
    bg: string;         // Background color (hex, e.g. "#89b4fa")
    bold: boolean;      // Bold text
}

function entry(ftb: FtbAPI): StatusBarSegment[] {
    return [
        { text: " main ", fg: "#1e1e2e", bg: "#a6e3a1", bold: true }
    ];
}
```

## Plugin Structure

### package.json

```json
{
    "name": "my-plugin",
    "version": "1.0.0",
    "description": "What this plugin does",
    "author": "Your Name",
    "entry": "main.ts",
    "type": "functional",
    "permissions": {
        "fs_read": true,
        "fs_write": false,
        "fs_list": true,
        "net_fetch": false,
        "env_read": false,
        "clipboard": false,
        "subprocess": false,
        "max_exec_ms": 5000
    },
    "keywords": [".md", "text/markdown"],
    "min_ftb_version": "0.1.0"
}
```

### main.ts

```typescript
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
        writeFile: (path: string, content: string) => void;
        listDir: (path: string) => string[];
    };
    clipboard: {
        read: () => string;
        write: (text: string) => void;
    };
    env: {
        get: (key: string) => string;
    };
    ui: {
        message: (text: string) => void;
        confirm: (text: string) => boolean;
    };
    statusBar: {
        set: (segments: StatusBarSegment[]) => void;
    };
    log: (msg: string) => void;
    exec: (cmd: string, args: string[]) => string;
}

function entry(ftb: FtbAPI): Record<string, any> {
    ftb.ui.message("Hello from my plugin!");
    return { success: true };
}
```

## API Reference

### `ftb.ctx` - Execution Context

| Field | Type | Description |
|-------|------|-------------|
| `current_path` | string | Current working directory |
| `selected_file` | string | Selected file name |
| `selected_file_path` | string | Full path of selected file |
| `selected_is_dir` | boolean | Is the selection a directory? |
| `selected_size` | number | File size in bytes |
| `selected_mime` | string | MIME type (if known) |
| `args` | object | Arguments passed to the plugin |
| `plugin_dir` | string | Plugin directory path (for `python.call` module resolution) |

### `ftb.fs` - File System (requires permissions)

| Method | Permission | Description |
|--------|-----------|-------------|
| `readFile(path)` | `fs_read` | Read file contents |
| `writeFile(path, content)` | `fs_write` | Write file contents |
| `listDir(path)` | `fs_list` | List directory entries |

### `ftb.clipboard` - Clipboard (requires `clipboard`)

| Method | Description |
|--------|-------------|
| `read()` | Read clipboard text |
| `write(text)` | Write to clipboard |

### `ftb.env` - Environment (requires `env_read`)

| Method | Description |
|--------|-------------|
| `get(key)` | Get environment variable |

### `ftb.ui` - User Interface

| Method | Description |
|--------|-------------|
| `message(text)` | Show message to user |
| `confirm(text)` | Ask user for confirmation |

### `ftb.statusBar` - Status Bar (for `statusbar` type plugins)

| Method | Description |
|--------|-------------|
| `set(segments)` | Set status bar segments |

Each segment: `{ text: string, fg: string, bg: string, bold: boolean }`

### `ftb.log(msg)` - Logging

Write to FTB's debug log (only when `-l` flag is used).

### `ftb.exec(cmd, args)` - Subprocess (requires `subprocess`)

Execute a command and return its output. **Use with caution**.

### `ftb.python.call(file, func, args)` - Python Call (requires `python_exec`)

Call a Python function from a module file in the plugin directory. Returns JSON result:

```typescript
// In utils.py:
// def compute(data, mode="stats"):
//     return {"sum": sum(data), "mean": sum(data)/len(data)}

const result = ftb.python.call("utils.py", "compute", { data: [1,2,3,4,5], mode: "stats" });
ftb.ui.message(`Mean: ${result.mean}`);
```

Parameters:
- `file` — Python file path (relative to plugin dir, e.g. `"utils.py"` or `"subdir/module.py"`)
- `func` — Function name
- `args` — Arguments object (passed as **kwargs to the function)
- Returns — Python function's return dict, serialized as JSON

Python code runs in an isolated `python3` subprocess, bounded by `max_exec_ms`.

## Permissions

Plugins run in a sandboxed environment. Each permission must be explicitly declared in `package.json`:

| Permission | Default | Description |
|-----------|---------|-------------|
| `fs_read` | `false` | Read files |
| `fs_write` | `false` | Write/modify files |
| `fs_list` | `false` | List directory contents |
| `net_fetch` | `false` | Make HTTP requests |
| `env_read` | `false` | Read environment variables |
| `clipboard` | `false` | Access system clipboard |
| `subprocess` | `false` | Execute subprocesses |
| `python_exec` | `false` | Execute Python code (`ftb.python.call`) |
| `max_exec_ms` | `5000` | Maximum execution time (ms) |

## Security Model

1. **Sandboxed Execution**: Plugins run in an isolated QuickJS process
2. **Permission-Based Access**: APIs are only available if declared in `package.json`
3. **Resource Limits**: Execution time is bounded by `max_exec_ms`
4. **Process Isolation**: Plugins cannot access FTB's memory space
5. **No Network by Default**: `net_fetch` must be explicitly granted

## Runtime Requirements

Plugins require [QuickJS](https://bellard.org/quickjs/) (`qjs`) to be installed:

```bash
# Ubuntu/Debian
sudo apt-get install quickjs

# From source
git clone https://github.com/bellard/quickjs
cd quickjs && make && sudo make install
```

TypeScript files are automatically transpiled to JavaScript before execution. For complex TypeScript, install `esbuild` for better compilation:

```bash
npm install -g esbuild
```

Plugins using `ftb.python.call()` or Python-based previewers also require **Python 3** (`python3`):

```bash
# Most systems have Python 3 pre-installed
python3 --version
```

## Examples

See the `plugins/` directory for example plugins:

- **hello-world.ftb** - Basic greeting plugin (`functional`)
- **file-info.ftb** - File information with size formatting (`functional`)
- **git-status.ftb** - Git branch and status in status bar (`statusbar`)
- **preview-external.ftb** - File preview using external tools + Python APK binary analysis (`previewer`)

### git-status Plugin

The `git-status` plugin automatically displays git information in the status bar when inside a git repository:

- **Branch name** (green when clean, blue when dirty)
- **Staged changes** (`+N` in yellow)
- **Modified files** (`~N` in orange)
- **Conflicts** (`!N` in red)
- **Untracked files** (`?N` in gray)

Requires permissions: `fs_read`, `fs_list`, `subprocess`

## Plugin Manager Shortcuts

| Key | Action |
|-----|--------|
| `j`/`k` | Navigate plugins |
| `Enter` | Execute selected plugin |
| `d` | Toggle enable/disable |
| `r` | Reload plugin |
| `q`/`Esc` | Close panel |

---

See also: [Configuration Guide](CONFIGURATION.md) | [配置指南](CONFIGURATION_CN.md)
