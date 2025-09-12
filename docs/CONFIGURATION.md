# FTB 配置文件说明

FTB使用`.ftb`配置文件来自定义界面样式、颜色主题和行为设置。配置文件位于用户主目录下，文件名为`.ftb`。

## 配置文件位置

- **Linux/macOS**: `~/.ftb`
- **Windows**: `%USERPROFILE%\.ftb`

## 配置文件格式

配置文件使用INI格式，包含多个节（section）和键值对。每个节用方括号包围，键值对用等号分隔。

## 配置节说明

### 1. 主界面颜色配置 `[colors.main]`

```ini
[colors.main]
# 背景色
background = black
# 前景色（文字颜色）
foreground = white
# 边框颜色
border = blue
# 选中项背景色
selection_bg = blue
# 选中项前景色
selection_fg = white
```

**支持的颜色值**:
- `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`
- `bright_black`, `bright_red`, `bright_green`, `bright_yellow`, `bright_blue`, `bright_magenta`, `bright_cyan`, `bright_white`

### 2. 文件类型颜色配置 `[colors.files]`

```ini
[colors.files]
# 文件夹颜色
directory = blue
# 普通文件颜色
file = white
# 可执行文件颜色
executable = green
# 链接文件颜色
link = cyan
# 隐藏文件颜色
hidden = yellow
# 系统文件颜色
system = red
```

### 3. 状态栏颜色配置 `[colors.status]`

```ini
[colors.status]
# 状态栏背景色
background = blue
# 状态栏前景色
foreground = white
# 时间显示颜色
time = yellow
# 路径显示颜色
path = cyan
```

### 4. 搜索框颜色配置 `[colors.search]`

```ini
[colors.search]
# 搜索框背景色
background = black
# 搜索框前景色
foreground = white
# 搜索框边框颜色
border = green
# 搜索高亮颜色
highlight = yellow
```

### 5. 对话框颜色配置 `[colors.dialog]`

```ini
[colors.dialog]
# 对话框背景色
background = black
# 对话框前景色
foreground = white
# 对话框边框颜色
border = blue
# 按钮背景色
button_bg = blue
# 按钮前景色
button_fg = white
# 输入框背景色
input_bg = black
# 输入框前景色
input_fg = white
```

### 6. 界面样式配置 `[style]`

```ini
[style]
# 是否显示图标
show_icons = true
# 是否显示文件大小
show_file_size = true
# 是否显示修改时间
show_modified_time = true
# 是否显示权限信息
show_permissions = true
# 是否启用鼠标支持
enable_mouse = true
# 是否启用动画效果
enable_animations = true
```

### 7. 布局配置 `[layout]`

```ini
[layout]
# 每页显示的项目数量
items_per_page = 20
# 每行显示的项目数量
items_per_row = 5
# 详情面板宽度比例 (0.0-1.0)
detail_panel_ratio = 0.3
# 是否显示详情面板
show_detail_panel = true
```

### 8. 刷新配置 `[refresh]`

```ini
[refresh]
# UI刷新间隔 (毫秒)
ui_refresh_interval = 100
# 目录内容刷新间隔 (毫秒)
content_refresh_interval = 1000
# 是否自动刷新
auto_refresh = true
```

### 9. 主题配置 `[theme]`

```ini
[theme]
# 主题名称
name = default
# 是否使用彩色输出
use_colors = true
# 是否使用粗体文字
use_bold = false
# 是否使用下划线
use_underline = false
```

**预定义主题**:
- `default`: 默认主题
- `dark`: 暗色主题
- `light`: 亮色主题
- `colorful`: 彩色主题
- `minimal`: 极简主题

### 10. MySQL连接配置 `[mysql]`

```ini
[mysql]
# 默认主机
default_host = localhost
# 默认端口
default_port = 3306
# 默认用户名
default_username = root
# 默认数据库
default_database = 
# 连接超时时间 (秒)
connection_timeout = 10
```

### 11. SSH连接配置 `[ssh]`

```ini
[ssh]
# 默认端口
default_port = 22
# 连接超时时间 (秒)
connection_timeout = 30
# 是否保存连接历史
save_connection_history = true
# 最大连接历史数量
max_connection_history = 10
```

### 12. 日志配置 `[logging]`

```ini
[logging]
# 日志级别
level = info
# 是否输出到文件
output_to_file = false
# 日志文件路径
log_file = ~/.ftb.log
# 是否显示时间戳
show_timestamp = true
```

**日志级别**:
- `debug`: 调试信息
- `info`: 一般信息
- `warning`: 警告信息
- `error`: 错误信息

## 配置示例

### 暗色主题配置

```ini
[colors.main]
background = black
foreground = white
border = gray
selection_bg = dark_gray
selection_fg = white

[colors.files]
directory = cyan
file = white
executable = green
link = yellow
hidden = dark_gray
system = red

[theme]
name = dark
use_colors = true
use_bold = false
use_underline = false
```

### 亮色主题配置

```ini
[colors.main]
background = white
foreground = black
border = dark_gray
selection_bg = blue
selection_fg = white

[colors.files]
directory = blue
file = black
executable = green
link = cyan
hidden = dark_gray
system = red

[theme]
name = light
use_colors = true
use_bold = false
use_underline = false
```

### 彩色主题配置

```ini
[colors.main]
background = black
foreground = white
border = magenta
selection_bg = magenta
selection_fg = white

[colors.files]
directory = cyan
file = white
executable = green
link = yellow
hidden = magenta
system = red

[colors.status]
background = magenta
foreground = white
time = yellow
path = cyan

[theme]
name = colorful
use_colors = true
use_bold = false
use_underline = false
```

## 配置热重载

FTB支持配置热重载，您可以在程序运行时修改配置文件，然后使用以下快捷键重新加载：

- **Ctrl+R**: 重新加载配置文件
- **Ctrl+T**: 切换主题

## 配置文件验证

FTB会自动验证配置文件的格式和值。如果发现无效的配置项，程序会：

1. 输出警告信息
2. 使用默认值
3. 继续正常运行

## 故障排除

### 配置文件无法加载

1. 检查文件权限：确保`.ftb`文件可读
2. 检查文件格式：确保使用正确的INI格式
3. 检查文件路径：确保文件位于用户主目录

### 颜色显示异常

1. 检查颜色值：确保使用支持的颜色名称
2. 检查终端支持：确保终端支持彩色输出
3. 检查主题设置：尝试切换到其他主题

### 配置不生效

1. 重新加载配置：使用Ctrl+R重新加载
2. 重启程序：完全退出并重新启动FTB
3. 检查配置语法：确保没有语法错误

## 高级配置

### 自定义颜色

您可以使用RGB值定义自定义颜色：

```ini
[colors.custom]
my_color = #FF5733
```

### 条件配置

某些配置项可以根据系统环境自动调整：

```ini
[colors.main]
# 根据终端类型自动选择颜色
background = auto
foreground = auto
```

## 配置文件备份

建议定期备份您的配置文件：

```bash
cp ~/.ftb ~/.ftb.backup
```

## 获取帮助

如果您在配置过程中遇到问题，可以：

1. 查看程序输出的错误信息
2. 检查配置文件语法
3. 参考本文档的配置示例
4. 重置为默认配置并重新开始 