# 🚀 FTB 快速配置指南

## ⚡ 5分钟快速配置

### 1. 复制配置文件模板
```bash
cp config/.ftb.template ~/.ftb
```

### 2. 编辑配置文件
```bash
nano ~/.ftb
# 或者使用你喜欢的编辑器
```

### 3. 快速主题切换
在配置文件中找到 `[theme]` 节，修改 `name` 值：
```ini
[theme]
name = colorful  # 可选: default, dark, light, colorful, minimal
```

### 4. 自定义颜色
在 `[colors.main]` 节中修改主界面颜色：
```ini
[colors.main]
background = black
foreground = white
border = blue
selection_bg = blue
selection_fg = white
```

### 5. 保存并重启
保存文件后重启FTB，新配置立即生效！

## 🎨 预定义主题速览

| 主题 | 特点 | 适用场景 |
|------|------|----------|
| **default** | 经典蓝白配色 | 日常使用 |
| **dark** | 深色背景高对比 | 夜间使用 |
| **light** | 浅色背景 | 明亮环境 |
| **colorful** | 缤纷色彩 | 视觉偏好 |
| **minimal** | 极简黑白 | 专注工作 |

## ⌨️ 运行时快捷键

- **Ctrl+T**: 循环切换主题
- **Ctrl+R**: 重新加载配置
- **Alt+D**: 打开MySQL管理

## 🔧 常用配置项

### 布局调整
```ini
[layout]
items_per_page = 25      # 每页显示25个项目
items_per_row = 6        # 每行显示6个项目
detail_panel_ratio = 0.4 # 详情面板占40%宽度
```

### 刷新设置
```ini
[refresh]
ui_refresh_interval = 50        # UI刷新更快
content_refresh_interval = 500  # 内容刷新适中
```

### 样式开关
```ini
[style]
show_icons = true        # 显示文件图标
enable_animations = true # 启用动画效果
enable_mouse = true      # 启用鼠标支持
```

## 🚨 常见问题

### Q: 配置文件不生效？
A: 检查文件权限和语法，使用 `Ctrl+R` 重载配置

### Q: 颜色显示异常？
A: 确保终端支持彩色输出，尝试切换主题

### Q: 如何恢复默认配置？
A: 删除 `~/.ftb` 文件，程序会自动使用默认配置

## 📚 更多配置选项

查看完整配置说明：[CONFIGURATION.md](docs/CONFIGURATION.md)

## 🎯 配置示例

### 程序员专用配置
```ini
[theme]
name = dark

[colors.main]
background = black
foreground = green
border = cyan
selection_bg = dark_gray

[style]
show_icons = false
enable_animations = false
```

### 设计师专用配置
```ini
[theme]
name = colorful

[colors.main]
background = black
foreground = white
border = magenta

[style]
show_icons = true
enable_animations = true
```

## 🔄 配置热重载

修改配置文件后，无需重启程序：
1. 保存配置文件
2. 按 `Ctrl+R` 重载配置
3. 新配置立即生效！

## 📱 移动端友好配置

```ini
[layout]
items_per_page = 15
items_per_row = 3
detail_panel_ratio = 0.5

[style]
enable_mouse = true
enable_animations = false
```

---

**🎉 恭喜！你已经掌握了FTB配置系统的基本用法！**

现在可以开始自定义你的专属界面了！ 