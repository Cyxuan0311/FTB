# FTB 插件系统

FTB 支持基于 TypeScript/JavaScript 的插件系统（灵感来源于 [yazi](https://yazi-rs.github.io/docs/plugins/overview)），允许你使用自定义脚本来扩展功能。

## 快速开始

### 安装插件

将插件目录放置在 `~/.config/ftb/plugins/` 中：

```
~/.config/ftb/plugins/
├── hello-world.ftb/
│   ├── main.ts          # 入口文件
│   └── package.json     # 元数据和权限声明
└── file-info.ftb/
    ├── main.ts
    └── package.json
```

### 打开插件管理器

按 `Ctrl+B` 然后输入 `:plugin` 或 `:pl`。

### 运行插件

在插件管理器面板中，选择一个插件并按 `Enter`。

## 插件类型

| 类型 | 说明 | 入口函数 | 返回值 |
|------|-------------|----------------|--------|
| `functional` | 执行一个操作 | `entry(ftb)` | `Record<string, any>` |
| `previewer` | 自定义文件预览 | `entry(ftb)` | `Record<string, any>` |
| `fetcher` | 异步元数据获取 | `entry(ftb)` | `Record<string, any>` |
| `statusbar` | 状态栏显示 | `entry(ftb)` | `StatusBarSegment[]` |

### StatusBar 插件

StatusBar 插件每 2 秒自动运行，为状态栏提供内容。入口函数必须返回一个片段对象数组：

```typescript
interface StatusBarSegment {
    text: string;       // 显示文本
    fg: string;         // 前景色（十六进制，例如 "#cdd6f4"）
    bg: string;         // 背景色（十六进制，例如 "#89b4fa"）
    bold: boolean;      // 粗体文字
}

function entry(ftb: FtbAPI): StatusBarSegment[] {
    return [
        { text: " main ", fg: "#1e1e2e", bg: "#a6e3a1", bold: true }
    ];
}
```

## 插件结构

### package.json

```json
{
    "name": "my-plugin",
    "version": "1.0.0",
    "description": "插件功能介绍",
    "author": "你的名字",
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
    ftb.ui.message("你好，来自我的插件！");
    return { success: true };
}
```

## API 参考

### `ftb.ctx` - 执行上下文

| 字段 | 类型 | 说明 |
|-------|------|------|
| `current_path` | string | 当前工作目录 |
| `selected_file` | string | 选中的文件名 |
| `selected_file_path` | string | 选中文件的完整路径 |
| `selected_is_dir` | boolean | 选中项是否为目录 |
| `selected_size` | number | 文件大小（字节） |
| `selected_mime` | string | MIME 类型（如果已知） |
| `args` | object | 传递给插件的参数 |
| `plugin_dir` | string | 插件目录路径（用于 `python.call` 解析模块路径） |

### `ftb.fs` - 文件系统（需要权限）

| 方法 | 所需权限 | 说明 |
|--------|-----------|------|
| `readFile(path)` | `fs_read` | 读取文件内容 |
| `writeFile(path, content)` | `fs_write` | 写入文件内容 |
| `listDir(path)` | `fs_list` | 列出目录内容 |

### `ftb.clipboard` - 剪贴板（需要 `clipboard` 权限）

| 方法 | 说明 |
|--------|------|
| `read()` | 读取剪贴板文本 |
| `write(text)` | 写入剪贴板 |

### `ftb.env` - 环境变量（需要 `env_read` 权限）

| 方法 | 说明 |
|--------|------|
| `get(key)` | 获取环境变量 |

### `ftb.ui` - 用户界面

| 方法 | 说明 |
|--------|------|
| `message(text)` | 向用户显示消息 |
| `confirm(text)` | 请求用户确认 |

### `ftb.statusBar` - 状态栏（仅 `statusbar` 类型插件）

| 方法 | 说明 |
|--------|------|
| `set(segments)` | 设置状态栏片段 |

每个片段：`{ text: string, fg: string, bg: string, bold: boolean }`

### `ftb.log(msg)` - 日志记录

写入 FTB 的调试日志（仅在使用 `-l` 标志时生效）。

### `ftb.exec(cmd, args)` - 子进程（需要 `subprocess` 权限）

执行命令并返回输出。**请谨慎使用**。

### `ftb.python.call(file, func, args)` - Python 调用（需要 `python_exec` 权限）

调用插件目录下的 Python 模块函数，返回 JSON 结果。Python 文件放在插件目录中，TS 入口文件可按需调用：

```typescript
// utils.py 中定义:
// def compute(data, mode="stats"):
//     return {"sum": sum(data), "mean": sum(data)/len(data)}

const result = ftb.python.call("utils.py", "compute", { data: [1,2,3,4,5], mode: "stats" });
ftb.ui.message(`Mean: ${result.mean}`);
```

参数说明：
- `file` — Python 文件路径（相对于插件目录，如 `"utils.py"` 或 `"subdir/module.py"`）
- `func` — 函数名
- `args` — 传递给函数的参数对象（作为 **kwargs 解包）
- 返回值 — Python 函数返回的字典，自动序列化为 JSON

Python 代码在独立的 `python3` 子进程中运行，受 `max_exec_ms` 超时限制。

## 权限

插件在沙箱环境中运行。每个权限必须在 `package.json` 中显式声明：

| 权限 | 默认值 | 说明 |
|-----------|---------|------|
| `fs_read` | `false` | 读取文件 |
| `fs_write` | `false` | 写入/修改文件 |
| `fs_list` | `false` | 列出目录内容 |
| `net_fetch` | `false` | 发起 HTTP 请求 |
| `env_read` | `false` | 读取环境变量 |
| `clipboard` | `false` | 访问系统剪贴板 |
| `subprocess` | `false` | 执行子进程 |
| `python_exec` | `false` | 执行 Python 代码（`ftb.python.call`） |
| `max_exec_ms` | `5000` | 最大执行时间（毫秒） |

## 安全模型

1. **沙箱执行**: 插件在隔离的 QuickJS 进程中运行
2. **基于权限的访问**: 只有在 `package.json` 中声明的 API 才可用
3. **资源限制**: 执行时间受 `max_exec_ms` 限制
4. **进程隔离**: 插件无法访问 FTB 的内存空间
5. **默认无网络**: `net_fetch` 必须显式授权

## 运行时要求

插件需要安装 [QuickJS](https://bellard.org/quickjs/)（`qjs`）：

```bash
# Ubuntu/Debian
sudo apt-get install quickjs

# 从源码编译
git clone https://github.com/bellard/quickjs
cd quickjs && make && sudo make install
```

TypeScript 文件在执行前会自动编译为 JavaScript。对于复杂的 TypeScript 项目，建议安装 `esbuild` 以获得更好的编译效果：

```bash
npm install -g esbuild
```

使用 `ftb.python.call()` 或基于 Python 的预览器还需要 **Python 3**（`python3`）：

```bash
python3 --version
```

## 示例

参见 `plugins/` 目录中的示例插件：

- **hello-world.ftb** - 基本问候插件（`functional`）
- **file-info.ftb** - 带格式的文件信息插件（`functional`）
- **git-status.ftb** - 状态栏中的 Git 分支和状态（`statusbar`）
- **preview-external.ftb** - 使用外部工具 + Python APK 二进制分析的预览器（`previewer`）

### git-status 插件

`git-status` 插件会在 git 仓库中自动显示状态栏信息：

- **分支名**（干净时绿色，有改动时蓝色）
- **暂存更改**（`+N` 黄色）
- **修改文件**（`~N` 橙色）
- **冲突**（`!N` 红色）
- **未跟踪文件**（`?N` 灰色）

所需权限：`fs_read`、`fs_list`、`subprocess`

## 插件管理器快捷键

| 按键 | 功能 |
|-----|--------|
| `j`/`k` | 导航插件列表 |
| `Enter` | 执行选中插件 |
| `d` | 切换启用/禁用 |
| `r` | 重新加载插件 |
| `q`/`Esc` | 关闭面板 |

---

另见：[配置指南](CONFIGURATION_CN.md) | [Configuration Guide](CONFIGURATION.md)
