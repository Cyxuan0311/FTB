# FTB 配置指南

FTB v2.1.0 使用 JSON 格式的配置文件来定制外观和行为。

## 文件位置

- **默认路径**: `~/.config/ftb/ftb.json`
- **自定义路径**: `FTB --config /path/to/config.json`
- **模板文件**: `config/.ftb.template`

如果启动时配置文件不存在，FTB 会自动创建默认配置。

## 快速开始

```bash
cp config/.ftb.template ~/.config/ftb/ftb.json
# 用任意文本编辑器编辑
```

## 配置项说明

### 颜色

所有颜色值支持十六进制格式（`#RRGGBB`）和命名终端颜色。

#### 主界面 (`colors.main`)

```json
{
  "colors": {
    "main": {
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "border": "#45475a",
      "selection_bg": "#585b70",
      "selection_fg": "#cdd6f4"
    }
  }
}
```

| 字段 | 说明 |
|-------|------|
| `background` | 主背景色 |
| `foreground` | 默认文字颜色 |
| `border` | 边框和分隔线颜色 |
| `selection_bg` | 选中项背景 |
| `selection_fg` | 选中项文字颜色 |

#### 文件类型颜色 (`colors.files`)

```json
{
  "colors": {
    "files": {
      "directory": "#89b4fa",
      "file": "#cdd6f4",
      "executable": "#a6e3a1",
      "link": "#f5c2e7",
      "hidden": "#6c7086",
      "system": "#f38ba8"
    }
  }
}
```

| 字段 | 说明 |
|-------|------|
| `directory` | 目录名称 |
| `file` | 普通文件 |
| `executable` | 可执行文件 |
| `link` | 符号链接 |
| `hidden` | 隐藏文件（以 `.` 开头） |
| `system` | 系统文件 |

#### 语法高亮颜色 (`colors.syntax`)

```json
{
  "colors": {
    "syntax": {
      "keyword": "#cba6f7",
      "string": "#a6e3a1",
      "comment": "#6c7086",
      "number": "#fab387",
      "function": "#89b4fa",
      "type": "#f9e2af",
      "operator": "#89dceb",
      "preprocessor": "#f38ba8",
      "identifier": "#cdd6f4",
      "punctuation": "#6c7086",
      "property": "#89dceb",
      "tag": "#cba6f7",
      "attribute": "#f9e2af",
      "regex": "#fab387",
      "decorator": "#f38ba8",
      "line_number": "#585b70"
    }
  }
}
```

| 字段 | 说明 |
|-------|------|
| `keyword` | 语言关键字（if, for, class 等） |
| `string` | 字符串字面量 |
| `comment` | 注释 |
| `number` | 数字字面量 |
| `function` | 函数名 |
| `type` | 类型名 |
| `operator` | 运算符（+, -, == 等） |
| `preprocessor` | 预处理指令（#include, #define） |
| `identifier` | 通用标识符 |
| `punctuation` | 括号、逗号、分号 |
| `property` | 对象属性/字段 |
| `tag` | HTML/XML 标签 |
| `attribute` | HTML/XML 属性 |
| `regex` | 正则表达式 |
| `decorator` | 装饰器/注解 |
| `line_number` | 预览中的行号 |

#### 状态栏 (`colors.status`)

```json
{
  "colors": {
    "status": {
      "background": "#313244",
      "foreground": "#cdd6f4",
      "border": "#45475a"
    }
  }
}
```

#### 搜索 (`colors.search`)

```json
{
  "colors": {
    "search": {
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "border": "#89b4fa"
    }
  }
}
```

#### 对话框 (`colors.dialog`)

```json
{
  "colors": {
    "dialog": {
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "border": "#89b4fa"
    }
  }
}
```

### 状态栏 (`statusbar`)

```json
{
  "statusbar": {
    "style": "powerline",
    "show_position": true,
    "show_time": true,
    "show_clipboard": true,
    "use_bold": false
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `style` | string | `"powerline"` | 可选：`powerline`、`rounded`、`thin`、`arrow`、`bar`、`minimal` |
| `show_position` | bool | `true` | 显示选中/总数位置 |
| `show_time` | bool | `true` | 显示当前时间 |
| `show_clipboard` | bool | `true` | 显示剪贴板待粘贴数量 |
| `use_bold` | bool | `false` | 状态栏使用粗体 |

### 样式 (`style`)

```json
{
  "style": {
    "show_icons": true,
    "show_file_size": true,
    "show_modified_time": true,
    "show_permissions": true,
    "enable_mouse": true,
    "enable_animations": false,
    "show_hidden_files": false,
    "show_detail_panel": true,
    "sort_mode": "name_asc"
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `show_icons` | bool | `true` | 显示 Nerd Font 文件图标（需要对应字体） |
| `show_file_size` | bool | `true` | 显示文件大小 |
| `show_modified_time` | bool | `true` | 显示修改时间 |
| `show_permissions` | bool | `true` | 显示文件权限 |
| `enable_mouse` | bool | `true` | 启用鼠标支持 |
| `enable_animations` | bool | `false` | 启用 UI 动画 |
| `show_hidden_files` | bool | `false` | 显示隐藏文件（以 `.` 开头） |
| `show_detail_panel` | bool | `true` | 显示预览面板 |
| `sort_mode` | string | `"name_asc"` | 排序模式：`name_asc`、`name_desc`、`size_asc`、`size_desc`、`time_asc`、`time_desc`、`ext_asc`、`ext_desc` |

### 布局 (`layout`)

```json
{
  "layout": {
    "parent_ratio": 0.222,
    "current_ratio": 0.444,
    "preview_ratio": 0.333,
    "items_per_page": 0
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `parent_ratio` | double | `0.222` | 父目录列宽度比例（0.0 - 1.0） |
| `current_ratio` | double | `0.444` | 当前目录列宽度比例 |
| `preview_ratio` | double | `0.333` | 预览面板宽度比例 |
| `items_per_page` | int | `0` | 每页项数（0 = 根据终端高度自动调整） |

比例会自动归一化到总和为 1.0。

### UI (`ui`)

```json
{
  "ui": {
    "column_separator": "thin",
    "panel_border": "rounded",
    "selection_style": "full",
    "tab_bar_style": "modern"
  }
}
```

| 字段 | 类型 | 默认值 | 可选值 |
|-------|------|---------|--------|
| `column_separator` | string | `"thin"` | `thin`、`light`、`heavy`、`double`、`dotted`、`dashed`、`none` |
| `panel_border` | string | `"rounded"` | `rounded`、`sharp`、`double`、`heavy`、`none` |
| `selection_style` | string | `"full"` | `full`、`underline`、`invert`、`bar`、`minimal` |
| `tab_bar_style` | string | `"modern"` | `modern`、`classic` |

### 刷新 (`refresh`)

```json
{
  "refresh": {
    "ui_refresh_interval_ms": 100,
    "content_refresh_interval_ms": 1000,
    "auto_refresh": true
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `ui_refresh_interval_ms` | int | `100` | UI 刷新间隔（毫秒） |
| `content_refresh_interval_ms` | int | `1000` | 目录内容刷新间隔 |
| `auto_refresh` | bool | `true` | 自动刷新目录内容 |

### 主题 (`theme`)

```json
{
  "theme": {
    "name": "default",
    "use_bold": false,
    "use_underline": false
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `name` | string | `"default"` | 主题名称（50+ 主题可用） |
| `use_bold` | bool | `false` | 使用粗体文字 |
| `use_underline` | bool | `false` | 使用下划线文字 |

**可用主题**: `default`、`yazi`、`dark`、`light`、`colorful`、`minimal`、`dracula`、`nord`、`tokyo-night`、`gruvbox`、`solarized`、`one-dark`、`rose-pine`、`kanagawa`、`everforest`、`monokai`、`ayu`、`poimandres`、`material`、`horizon`、`melange`、`solarized-light`、`macchiato`、`github`、`synthwave`、`nightfox`、`tokyo-storm`、`rose-pine-dawn`、`gruvbox-light`、`oxocarbon`、`ayu-mirage`、`everforest-light`、`andromeda`、`frappe`、`doom-one`、`outrun`、`falcon`、`vscode`、`moonfly`、`tomorrow-night`、`zenburn`、`latte`、`palenight`、`monokai-pro`、`flexoki`、`one-light`、`nord-light`、`challenger`、`cobalt2`、`iceberg` 等。

设置主题后，其颜色会覆盖 `colors` 部分。当 `theme.name` 未设置时，`colors` 部分的自定义颜色生效；你也可以在设置主题后覆盖特定字段。

### SSH (`ssh`)

```json
{
  "ssh": {
    "default_port": 22,
    "connection_timeout": 30,
    "save_connection_history": true,
    "max_connection_history": 10
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `default_port` | int | `22` | 默认 SSH 端口 |
| `connection_timeout` | int | `30` | 连接超时（秒） |
| `save_connection_history` | bool | `true` | 保存连接历史 |
| `max_connection_history` | int | `10` | 最大保存连接数 |

### 日志 (`logging`)

```json
{
  "logging": {
    "level": "info",
    "output_to_file": false,
    "log_file": "~/.config/ftb/ftb.log",
    "show_timestamp": true
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `level` | string | `"info"` | 日志级别（`info`、`debug`、`warn`、`error`） |
| `output_to_file` | bool | `false` | 写入日志文件 |
| `log_file` | string | `"~/.config/ftb/ftb.log"` | 日志文件路径 |
| `show_timestamp` | bool | `true` | 在日志中包含时间戳 |

### 预览 (`preview`)

```json
{
  "preview": {
    "max_text_file_size_kb": 0,
    "max_text_lines": 0,
    "max_dir_entries": 0,
    "max_hex_bytes": 0,
    "max_archive_nodes": 0,
    "max_spreadsheet_rows": 0,
    "truncate_long_lines": true,
    "chunk_size_lines": 200,
    "virtual_scroll_margin": 50
  }
}
```

| 字段 | 类型 | 默认值 | 说明 |
|-------|------|---------|------|
| `max_text_file_size_kb` | int | `0` | 文本预览最大文件大小（KB，0 = 不限制） |
| `max_text_lines` | int | `0` | 文本预览最大行数（0 = 不限制） |
| `max_dir_entries` | int | `0` | 显示的最大目录项数（0 = 不限制） |
| `max_hex_bytes` | int | `0` | 十六进制转储最大字节数（0 = 不限制） |
| `max_archive_nodes` | int | `0` | 显示的最大归档条目数（0 = 不限制） |
| `max_spreadsheet_rows` | int | `0` | 电子表格最大行数（0 = 不限制） |
| `truncate_long_lines` | bool | `true` | 截断超出面板宽度的行 |
| `chunk_size_lines` | int | `200` | 延迟加载文本的每块行数 |
| `virtual_scroll_margin` | int | `50` | 视口上下预加载的额外行数 |

### 书签 (`bookmarks`)

```json
{
  "bookmarks": {
    "home": "~",
    "root": "/",
    "tmp": "/tmp"
  }
}
```

命名目录书签。使用 `:jump` 命令（`Ctrl+B j`）跳转到书签。

### 打开器 (`opener`)

```json
{
  "opener": {
    "openers": {
      "edit": [{"run": "", "block": true, "desc": "内置编辑器"}],
      "open": [
        {"run": "xdg-open %s", "orphan": true, "desc": "系统默认", "platform": "linux"},
        {"run": "open %s", "orphan": true, "desc": "系统默认", "platform": "macos"}
      ]
    },
    "rules": [
      {"name": "*", "use": ["open"]}
    ]
  }
}
```

| 字段 | 说明 |
|-------|------|
| `openers` | 定义打开器程序（edit、open 等） |
| `rules` | 将文件模式映射到打开器列表 |

`*` 通配符规则匹配所有文件。规则按顺序评估，第一个匹配生效。

### 无预览扩展名

`config/noavilable.json` 中列出的扩展名不会触发预览：

```json
{
    "no_preview_extensions": [
        ".unitypackage"
    ]
}
```

## 热重载

按 `Ctrl+R` 重新加载配置文件，无需重启 FTB。

## CLI 参数

| 参数 | 说明 |
|----------|------|
| `-h`、`--help` | 显示帮助 |
| `-v`、`--version` | 显示版本（v2.1.0） |
| `--config <PATH>` | 指定配置文件路径 |
| `--no-icons` | 构建选项：使用 `-DFTB_ENABLE_ICONS=OFF` 重新构建 |
| `-l` | 启用性能调试日志到 `ftb_perf.log` |
| `[DIRECTORY]` | 启动目录 |

## 颜色格式

支持的颜色格式：

- **十六进制**: `"#RRGGBB"`（例如 `"#89b4fa"`）
- **命名颜色**: 终端颜色名称（例如 `"black"`、`"red"`、`"blue"`、`"white"` 等）
- **亮色变体**: `"bright_black"`、`"bright_red"` 等

---

另见：[键盘快捷键](KEYBOARD_SHORTCUTS_CN.md) | [Keyboard Shortcuts](KEYBOARD_SHORTCUTS.md)
