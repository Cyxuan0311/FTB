# FTB Configuration Guide

FTB uses a JSON configuration file (`~/.ftb`) to customize appearance and behavior.

## File Location

- **Default**: `~/.ftb`
- **Custom path**: `./FTB --config /path/to/config.ftb`
- **Template**: `config/.ftb.template`

## Quick Start

```bash
cp config/.ftb.template ~/.ftb
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
      "system": "#f38ba8"
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

### Style

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
    "show_detail_panel": true
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

### Layout

```json
{
  "layout": {
    "parent_column_width": 25,
    "detail_panel_ratio": 0.3,
    "items_per_page": 0
  }
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `parent_column_width` | int | `25` | Width of the parent directory column (characters) |
| `detail_panel_ratio` | double | `0.3` | Preview panel width ratio (0.0 - 1.0) |
| `items_per_page` | int | `0` | Items per page (0 = auto based on terminal height) |

### Refresh

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

### Theme

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
| `name` | string | `"default"` | Theme name (see list below) |
| `use_bold` | bool | `false` | Use bold text |
| `use_underline` | bool | `false` | Use underlined text |

**Available themes**: `default`, `yazi`, `dark`, `light`, `colorful`, `minimal`, `dracula`, `nord`, `tokyo-night`, `gruvbox`, `solarized`, `one-dark`, `rose-pine`, `kanagawa`, `everforest`, `monokai`, `ayu`, `poimandres`, `material`, `horizon`, `melange`, `solarized-light`

When a theme is set, its colors override the `colors` section. Custom colors in the `colors` section take effect when `theme.name` is not set or when you override specific fields after setting a theme.

### SSH

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

### Bookmarks

```json
{
  "bookmarks": {
    "home": "~",
    "root": "/",
    "tmp": "/tmp"
  }
}
```

Named directory bookmarks. Jump to a bookmark using the `:jump` command.

## Hot Reload

Press `Ctrl+R` to reload the configuration file without restarting FTB.

## Color Format

Supported color formats:

- **Hex**: `"#RRGGBB"` (e.g., `"#89b4fa"`)
- **Named**: Terminal color names (e.g., `"black"`, `"red"`, `"blue"`, `"white"`, etc.)
- **Bright variants**: `"bright_black"`, `"bright_red"`, etc.

---

See also: [Keyboard Shortcuts](KEYBOARD_SHORTCUTS.md)
