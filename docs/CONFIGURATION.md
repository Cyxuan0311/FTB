[中文文档](CONFIGURATION_CN.md)  |  English

# FTB Configuration Guide

FTB v2.1.0 uses a JSON configuration file to customize appearance and behavior.

## File Location

- **Default**: `~/.config/ftb/ftb.json`
- **Custom path**: `FTB --config /path/to/config.json`
- **Template**: `config/.ftb.template`

If the config file does not exist at startup, FTB creates one with default values.

## Quick Start

```bash
cp config/.ftb.template ~/.config/ftb/ftb.json
# Edit with any text editor
```

## Configuration Sections

### Colors

All color values support hex format (`#RRGGBB`) and named terminal colors.

#### Main Interface (`colors.main`)

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

| Field | Description |
|-------|-------------|
| `background` | Main background color |
| `foreground` | Default text color |
| `border` | Border and separator color |
| `selection_bg` | Selected item background |
| `selection_fg` | Selected item text color |

#### File Type Colors (`colors.files`)

```json
{
  "colors": {
    "files": {
      "directory": "#89b4fa",
      "file": "#cdd6f4",
      "executable": "#a6e3a1",
      "link": "#f5c2e7",
      "hidden": "#6c7086",
      "system": "#f38ba8",
      "extensions": {
        "cpp": "#a6e3a1",
        "hpp": "#a6e3a1",
        "py": "#89b4fa",
        "js": "#f5c2e7",
        "ts": "#f5c2e7",
        "md": "#89b4fa",
        "txt": "#6c7086",
        "toml": "#6c7086",
        "json": "#f9e2af",
        "yaml": "#f9e2af",
        "yml": "#f9e2af",
        "png": "#f38ba8",
        "jpg": "#f38ba8",
        "jpeg": "#f38ba8",
        "gif": "#f38ba8",
        "svg": "#f38ba8",
        "zip": "#f5c2e7",
        "tar": "#f5c2e7",
        "gz": "#f5c2e7"
      }
    }
  }
}
```

| Field | Description |
|-------|-------------|
| `directory` | Directory names |
| `file` | Regular files |
| `executable` | Executable files |
| `link` | Symbolic links |
| `hidden` | Hidden files (starting with `.`) |
| `system` | System files |
| `extensions` | Extension-to-color map (overrides categories for matching files) |

#### Syntax Highlighting Colors (`colors.syntax`)

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

| Field | Description |
|-------|-------------|
| `keyword` | Language keywords (if, for, class, etc.) |
| `string` | String literals |
| `comment` | Comments |
| `number` | Numeric literals |
| `function` | Function names |
| `type` | Type names |
| `operator` | Operators (+, -, ==, etc.) |
| `preprocessor` | Preprocessor directives (#include, #define) |
| `identifier` | General identifiers |
| `punctuation` | Brackets, commas, semicolons |
| `property` | Object properties / fields |
| `tag` | HTML/XML tags |
| `attribute` | HTML/XML attributes |
| `regex` | Regular expressions |
| `decorator` | Decorators / annotations |
| `line_number` | Line numbers in preview |

#### Status Bar (`colors.status`)

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

#### Search (`colors.search`)

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

#### Dialog (`colors.dialog`)

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

### Status Bar (`statusbar`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `style` | string | `"powerline"` | One of: `powerline`, `rounded`, `thin`, `arrow`, `bar`, `minimal` |
| `show_position` | bool | `true` | Show selected/total position |
| `show_time` | bool | `true` | Show current time |
| `show_clipboard` | bool | `true` | Show clipboard pending paste count |
| `use_bold` | bool | `false` | Use bold text in status bar |

### Style (`style`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `show_icons` | bool | `true` | Show Nerd Font file icons (requires patched font) |
| `show_file_size` | bool | `true` | Show file sizes |
| `show_modified_time` | bool | `true` | Show modification timestamps |
| `show_permissions` | bool | `true` | Show file permissions |
| `enable_mouse` | bool | `true` | Enable mouse support |
| `enable_animations` | bool | `false` | Enable UI animations |
| `show_hidden_files` | bool | `false` | Show hidden files (starting with `.`) |
| `show_detail_panel` | bool | `true` | Show the preview panel |
| `sort_mode` | string | `"name_asc"` | Sort mode: `name_asc`, `name_desc`, `size_asc`, `size_desc`, `time_asc`, `time_desc`, `ext_asc`, `ext_desc` |

### Layout (`layout`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `parent_ratio` | double | `0.222` | Parent column width ratio (0.0 - 1.0) |
| `current_ratio` | double | `0.444` | Current directory column width ratio |
| `preview_ratio` | double | `0.333` | Preview panel width ratio |
| `items_per_page` | int | `0` | Items per page (0 = auto based on terminal height) |

Ratios are normalized to sum to 1.0 automatically.

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

| Field | Type | Default | Values |
|-------|------|---------|--------|
| `column_separator` | string | `"thin"` | `thin`, `light`, `heavy`, `double`, `dotted`, `dashed`, `none` |
| `panel_border` | string | `"rounded"` | `rounded`, `sharp`, `double`, `heavy`, `none` |
| `selection_style` | string | `"full"` | `full`, `underline`, `invert`, `bar`, `minimal` |
| `tab_bar_style` | string | `"modern"` | `modern`, `classic` |

### Refresh (`refresh`)

```json
{
  "refresh": {
    "ui_refresh_interval_ms": 100,
    "content_refresh_interval_ms": 1000,
    "auto_refresh": true
  }
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `ui_refresh_interval_ms` | int | `100` | UI refresh interval in milliseconds |
| `content_refresh_interval_ms` | int | `1000` | Directory content refresh interval |
| `auto_refresh` | bool | `true` | Auto-refresh directory contents |

### Theme (`theme`)

```json
{
  "theme": {
    "name": "default",
    "use_bold": false,
    "use_underline": false
  }
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | string | `"default"` | Theme name (50+ themes available) |
| `use_bold` | bool | `false` | Use bold text |
| `use_underline` | bool | `false` | Use underlined text |

**Available themes**: `default`, `yazi`, `dark`, `light`, `colorful`, `minimal`, `dracula`, `nord`, `tokyo-night`, `gruvbox`, `solarized`, `one-dark`, `rose-pine`, `kanagawa`, `everforest`, `monokai`, `ayu`, `poimandres`, `material`, `horizon`, `melange`, `solarized-light`, `macchiato`, `github`, `synthwave`, `nightfox`, `tokyo-storm`, `rose-pine-dawn`, `gruvbox-light`, `oxocarbon`, `ayu-mirage`, `everforest-light`, `andromeda`, `frappe`, `doom-one`, `outrun`, `falcon`, `vscode`, `moonfly`, `tomorrow-night`, `zenburn`, `latte`, `palenight`, `monokai-pro`, `flexoki`, `one-light`, `nord-light`, `challenger`, `cobalt2`, `iceberg` and more.

When a theme is set, its colors override the `colors` section. Custom colors in the `colors` section take effect when `theme.name` is not set or when you override specific fields after setting a theme.

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `default_port` | int | `22` | Default SSH port |
| `connection_timeout` | int | `30` | Connection timeout in seconds |
| `save_connection_history` | bool | `true` | Save connection history |
| `max_connection_history` | int | `10` | Maximum saved connections |

### Logging (`logging`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `level` | string | `"info"` | Log level (`info`, `debug`, `warn`, `error`) |
| `output_to_file` | bool | `false` | Write logs to file |
| `log_file` | string | `"~/.config/ftb/ftb.log"` | Log file path |
| `show_timestamp` | bool | `true` | Include timestamps in logs |

### Preview (`preview`)

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `max_text_file_size_kb` | int | `0` | Max text file size for preview (0 = no limit) |
| `max_text_lines` | int | `0` | Max lines for text preview (0 = no limit) |
| `max_dir_entries` | int | `0` | Max directory entries to show (0 = no limit) |
| `max_hex_bytes` | int | `0` | Max bytes for hex dump (0 = no limit) |
| `max_archive_nodes` | int | `0` | Max archive entries to show (0 = no limit) |
| `max_spreadsheet_rows` | int | `0` | Max spreadsheet rows (0 = no limit) |
| `truncate_long_lines` | bool | `true` | Truncate lines exceeding panel width |
| `chunk_size_lines` | int | `200` | Lines per chunk for lazy text loading |
| `virtual_scroll_margin` | int | `50` | Extra lines to preload above/below viewport |

### Bookmarks (`bookmarks`)

```json
{
  "bookmarks": {
    "home": "~",
    "root": "/",
    "tmp": "/tmp"
  }
}
```

Named directory bookmarks. Jump to a bookmark using the `:jump` command (`Ctrl+B j`).

### Opener (`opener`)

```json
{
  "opener": {
    "openers": {
      "edit": [{"run": "", "block": true, "desc": "Built-in editor"}],
      "open": [
        {"run": "xdg-open %s", "orphan": true, "desc": "System default", "platform": "linux"},
        {"run": "open %s", "orphan": true, "desc": "System default", "platform": "macos"}
      ]
    },
    "rules": [
      {"name": "*", "use": ["open"]}
    ]
  }
}
```

| Field | Description |
|-------|-------------|
| `openers` | Define opener programs (edit, open, etc.) |
| `rules` | Map file patterns to opener lists |

The `*` wildcard rule matches all files. Rules are evaluated in order; the first match wins.

### No-Preview Extensions

Extensions listed in `config/noavilable.json` are excluded from preview:

```json
{
    "no_preview_extensions": [
        ".unitypackage"
    ]
}
```

## Hot Reload

Press `Ctrl+R` to reload the configuration file without restarting FTB.

## CLI Arguments

| Argument | Description |
|----------|-------------|
| `-h`, `--help` | Show help |
| `-v`, `--version` | Show version (v2.1.0) |
| `--config <PATH>` | Specify config file path |
| `--no-icons` | Build-time option: rebuild with `-DFTB_ENABLE_ICONS=OFF` |
| `-l` | Enable performance debug logging to `ftb_perf.log` |
| `[DIRECTORY]` | Startup directory |

## Color Format

Supported color formats:

- **Hex**: `"#RRGGBB"` (e.g., `"#89b4fa"`)
- **Named**: Terminal color names (e.g., `"black"`, `"red"`, `"blue"`, `"white"`, etc.)
- **Bright variants**: `"bright_black"`, `"bright_red"`, etc.

---

See also: [Keyboard Shortcuts](KEYBOARD_SHORTCUTS.md) | [中文](KEYBOARD_SHORTCUTS_CN.md)
