<div align="center">

# FTB - 终端文件浏览器

</div>

<div align="center">
  <img src="./resource/logo.png" alt="FTB Logo" width="200" height="200">
</div>

<div align="center">

![C++17](https://img.shields.io/badge/C++-17-blue) ![FTXUI](https://img.shields.io/badge/FTXUI-5.0.0-orange) ![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey) ![CMake](https://img.shields.io/badge/CMake-3.10+-red)

[English](README.md) | 中文

</div>

终端文件浏览器，基于 C++17 和 [FTXUI](https://github.com/ArthurSonzogni/ftxui) 构建。采用三栏布局（父目录 / 当前目录 / 预览），支持 Vim 风格快捷键、语法高亮预览和丰富的主题系统。

<div align="center">
  <img src="resource/demo.png" alt="plmux demo" />
  <p>plmux 在Windows终端中运行</p>
</div>

## 功能特性

### 三栏布局
- **左栏**：父目录文件列表
- **中栏**：当前目录，带文件图标和类型着色
- **右栏**：上下文感知预览面板（文件元信息、带语法高亮的文本、图片渲染、目录内容）

### 文件操作
- 复制 (Ctrl+Y)、剪切 (Ctrl+X)、粘贴 (Ctrl+V)
- 删除 (Delete / Ctrl+D)
- 新建文件/文件夹（通过命令模式）
- 重命名（通过命令模式）
- 剪贴板管理，带可视化指示器

### Vim 风格快捷键
- `j`/`k` 或方向键导航
- `h` 返回上级目录，`l` 进入目录
- `/` 搜索，`Ctrl+B` 前缀模式执行命令
- 内置 Vim 风格文本编辑器，支持撤销/重做、搜索/替换和 Markdown 预览

### 语法高亮
- 内置基于关键词的高亮器，支持 40+ 种语言（C/C++、Python、Rust、Go、Java、JavaScript、TypeScript 等）
- 16 种 Token 类型：关键字、字符串、注释、数字、函数、类型、操作符、预处理指令等
- 可选 tree-sitter 后端实现高级语法解析（使用 `-DFTB_ENABLE_TREE_SITTER=ON` 编译）
- 主题感知颜色，随当前主题自动适配

### 图片预览
- 使用 Unicode 半块字符在终端中渲染 JPG、PNG、BMP、GIF 图片
- 可选 libchafa 后端获得更高质量（使用 `-DFTB_ENABLE_LIBCHAFA=ON` 编译）

### 主题系统
18 个内置主题，均包含完整的语法高亮颜色支持：

| 主题 | 风格 | 主题 | 风格 |
|------|------|------|------|
| default | Catppuccin Mocha | yazi | Yazi 风格 |
| dark | 高对比度暗色 | light | 亮色背景 |
| colorful | 鲜艳色彩 | minimal | 单色极简 |
| dracula | Dracula | nord | Nord |
| tokyo-night | Tokyo Night | gruvbox | Gruvbox |
| solarized | Solarized Dark | one-dark | One Dark |
| rose-pine | Rose Pine | kanagawa | Kanagawa |
| everforest | Everforest | monokai | Monokai |
| ayu | Ayu Dark | poimandres | Poimandres |
| material | Material Dark | horizon | Horizon |
| melange | Melange | solarized-light | Solarized Light |
| ...  | ... | ... | ... |

- 通过命令模式 (`:theme`) 实时切换主题
- JSON 配置文件 (`~/.ftb`) 持久化自定义设置
- 每个主题独立的语法高亮颜色

### 命令模式（nvim 风格）
按 `Ctrl+B` 进入前缀模式，输入命令：

| 命令 | 别名 | 功能 |
|------|------|------|
| `:theme` | `:th` | 打开主题选择器 |
| `:rename` | `:rn` | 重命名选中项 |
| `:newfile` | `:nf` | 新建文件 |
| `:newfolder` | `:nd` | 新建文件夹 |
| `:preview` | `:p` | 完整文件预览 |
| `:details` | `:d` | 文件夹详情 |
| `:jump` | `:j` | 跳转到目录 |
| `:vim` | `:v` | 用 Vim 编辑器打开 |
| `:search` | `:s` | 进入搜索模式 |
| `:calendar` | `:cal` | 日历面板 |
| `:layout` | `:lo` | 布局设置 |
| `:help` | `:h` | 帮助面板 |
| `:plugin` | `:pl` | 插件管理器 |
| `:ssh` | - | SSH 连接（需启用） |
| ... | ... | ... |

### 性能优化
- 异步文件 I/O，UI 不阻塞
- LRU 缓存目录内容
- Vim 编辑器实例对象池
- 预览内容延迟加载
- 可配置的刷新间隔

### 插件系统
- TypeScript/JavaScript 插件，沙箱化执行
- 基于权限的 API（文件系统、剪贴板、环境变量、子进程）
- 插件管理面板（`:plugin`）
- 自动发现 `~/.config/ftb/plugins/` 下的插件
- 支持热重载
- 详见 [插件指南](docs/PLUGINS.md)

## 编译

### 依赖

| 库 | 用途 | 是否必须 |
|----|------|----------|
| FTXUI | 终端 UI 框架 | 是 |
| nlohmann-json | JSON 配置解析 | 是 |
| libssh2 | SSH 连接 | 否 (`-DFTB_ENABLE_SSH=ON`) |
| tree-sitter | 高级语法高亮 | 否 (`-DFTB_ENABLE_TREE_SITTER=ON`) |
| libchafa | 图片预览 | 否 (`-DFTB_ENABLE_LIBCHAFA=ON`) |

### 安装依赖（Ubuntu/Debian）

```bash
sudo apt-get update && sudo apt-get install -y \
    libftxui-dev nlohmann-json3-dev cmake g++
```

### 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 编译选项

```bash
# 禁用 Nerd Font 图标（使用 ASCII 回退）
cmake .. -DFTB_ENABLE_ICONS=OFF

# 启用 SSH 支持
cmake .. -DFTB_ENABLE_SSH=ON

# 启用 tree-sitter 语法高亮
cmake .. -DFTB_ENABLE_TREE_SITTER=ON

# 启用 libchafa 图片预览
cmake .. -DFTB_ENABLE_LIBCHAFA=ON

# 使用交互式脚本编译
chmod +x ./build.sh
./build.sh
```

## 使用

```bash
# 在当前目录启动
./FTB

# 在指定目录启动
./FTB /path/to/directory

# 启用性能日志
./FTB -l

# 指定配置文件
./FTB --config /path/to/config.ftb
```

### 快捷键

| 按键 | 功能 |
|------|------|
| `j` / `Down` | 向下移动 |
| `k` / `Up` | 向上移动 |
| `h` / `Left` | 返回上级目录 |
| `l` / `Right` / `Enter` | 进入目录 / 打开文件 |
| `/` | 搜索 |
| `Ctrl+B` | 命令模式 |
| `Ctrl+Y` | 复制 |
| `Ctrl+X` | 剪切 |
| `Ctrl+V` | 粘贴 |
| `Delete` / `Ctrl+D` | 删除 |
| `Ctrl+R` | 重新加载配置 |
| `Ctrl+C` | 退出 |
| `Escape` | 关闭面板 / 清除搜索 |
| `Home` / `End` | 跳转到首项 / 末项 |
| `PageUp` / `PageDown` | 翻页 |

完整快捷键参考见 [键盘快捷键](KEYBOARD_SHORTCUTS.md)。

## 配置

复制模板并编辑：

```bash
cp config/.ftb.template ~/.ftb
```

配置文件使用 JSON 格式，主要包含：

- `colors` - 界面颜色（主界面、文件类型、状态栏、搜索、对话框、语法高亮）
- `style` - 图标、文件大小、鼠标支持、隐藏文件
- `layout` - 栏目比例和每页项数
- `theme` - 当前主题名称
- `ssh` - 默认 SSH 端口和超时
- `bookmarks` - 命名目录书签

详见 [配置指南](CONFIGURATION.md)。

## 文档

- [键盘快捷键](KEYBOARD_SHORTCUTS.md)
- [配置指南](CONFIGURATION.md)
- [插件系统](docs/PLUGINS.md)

## 许可证

MIT
