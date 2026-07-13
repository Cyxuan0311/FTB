[中文文档](KEYBOARD_SHORTCUTS_CN.md)  |  English

# FTB Keyboard Shortcuts

## File List Navigation

| Key | Action |
|-----|--------|
| `j` / `Down` | Move selection down |
| `k` / `Up` | Move selection up |
| `h` / `Left` | Go to parent directory (first press = first item, second = parent) |
| `l` / `Right` / `Enter` | Enter directory / open file |
| `Home` / `g` `g` | Jump to first item |
| `End` / `G` | Jump to last item |
| `PageUp` | Previous page |
| `PageDown` | Next page |
| `.` | Toggle hidden files |
| `Space` | Toggle batch selection (multi-select) |

## File Operations

| Key | Action |
|-----|--------|
| `y` | Copy (yank) selected item(s) |
| `x` | Cut selected item(s) |
| `p` | Paste from clipboard (auto-rename if exists) |
| `P` | Paste with force overwrite |
| `d` | Move to trash |
| `Delete` / `Ctrl+D` | Open delete confirmation dialog |

## Search

| Key | Action |
|-----|--------|
| `/` | Enter search mode (type to filter list) |
| `Escape` | Clear search / exit search mode |
| `n` | Jump to next match |
| `N` | Jump to previous match |
| `Backspace` | Delete last character / exit if empty |

## Preview Panel Scrolling

| Key | Action |
|-----|--------|
| `Alt+J` | Scroll preview down |
| `Alt+K` | Scroll preview up |
| `Alt+H` | Scroll preview left |
| `Alt+L` | Scroll preview right |
| Mouse Wheel | Scroll preview up/down |

## Tab Management

| Key | Action |
|-----|--------|
| `[` | Previous tab |
| `]` | Next tab |
| `Ctrl+B nt` | Create new tab |
| `Ctrl+B ct` | Close current tab |

## Command Mode

Press `Ctrl+B` to enter command prefix mode, then type a command:

| Command | Aliases | Action |
|---------|---------|--------|
| `open` | `op` | Open with picker dialog |
| `openwith` | `ow` | Manual specify program |
| `opencfg` | `oc` | Configure openers |
| `vim` | `v`, `e`, `editor` | Open built-in editor (full-screen tab, Esc to close) |
| `theme` | `th` | Open theme selector |
| `rename` | `rn` | Rename selected item |
| `batchrename` | `br` | Batch rename with regex |
| `extract` | `ext` | Extract archive file |
| `newfile` | `nf` | Create new file |
| `newfolder` | `nd` | Create new folder |
| `preview` | `p` | Full file preview |
| `details` | `d` | Folder details |
| `jump` | `j` | Jump to directory |
| `fdfind` | `fd` | Fuzzy find files |
| `search` | `s` | Enter search mode |
| `help` | `h` | Help panel |
| `calendar` | `cal` | Calendar panel |
| `layout` | `lo` | Layout settings |
| `sort` | `so` | Sort mode settings |
| `uistyle` | `ui` | UI style settings |
| `statusstyle` | `ss` | Status bar style |
| `plugin` | `pl` | Plugin manager |
| `tasks` | `task`, `ts` | Task manager |
| `clipboard` | `cb` | Show clipboard details |
| `clr` | `cc` | Clear clipboard |
| `newtab` | `nt` | Create new tab |
| `closetab` | `ct` | Close current tab |
| `nexttab` | `nx` | Next tab |
| `prevtab` | `pv` | Previous tab |
| `mdsource` | `mds` | Toggle Markdown preview source |
| `xlsx` | `xls` | Toggle spreadsheet preview |
| `media` | `video`, `mp4`, `gif` | Toggle media (timg) preview |
| `timg` | `play` | Play media fullscreen |
| `pdf` | `hygg` | Toggle PDF preview |
| `doc` | `docx`, `pandoc` | Toggle DOC/DOCX preview |
| `aud` | `audio`, `eyed3` | Toggle audio preview |
| `hex` | `xxd` | Toggle hex dump preview |
| `gh` | | Go to home directory |
| `gd` | | Go to downloads directory |
| `gc` | | Go to config directory |
| `z` | `exit`, `quit` | Quit and cd to current directory in shell (requires shell wrapper) |
| `ssh` | | SSH connection (if enabled) |

- `Tab` — Auto-complete command name
- `Enter` — Execute command
- `Escape` — Cancel command mode

## Built-in Editor (nano-style)

Press `Ctrl+B v` or `Ctrl+B e` to open the current file in the built-in editor.

### Navigation

| Key | Action |
|-----|--------|
| Arrow keys | Move cursor |
| `Ctrl+A` / `Ctrl+E` | Beginning / End of line |
| `Ctrl+B` / `Ctrl+F` | Character left / right |
| `Ctrl+P` / `Ctrl+N` | Line up / down |
| `Ctrl+V` / `PageDown` | Page down |
| `Ctrl+Y` / `PageUp` | Page up |

### Editing

| Key | Action |
|-----|--------|
| `Ctrl+O` | Save file |
| `Ctrl+X` | Exit editor |
| `Ctrl+K` | Cut current line |
| `Ctrl+U` | Paste at cursor |
| `Ctrl+W` | Search within file |
| `Ctrl+Z` / `Alt+U` | Undo |
| `Alt+E` | Redo |
| `Alt+M` | Toggle Markdown preview mode |
| `Ctrl+_` | Go to specific line |
| `Ctrl+C` | Show cursor position |

## Global / Application Control

| Key | Action |
|-----|--------|
| `Escape` | Close active panel / clear search / clear batch selection |
| `Ctrl+C` | Quit FTB |
| `Ctrl+R` | Reload configuration file |
| `Ctrl+L` | Refresh terminal screen |
| `Ctrl+Z` | (ignored, prevents accidental suspend) |

## Panels (Help, Theme, Calendar, etc.)

| Key | Action |
|-----|--------|
| `Escape` / `q` | Close panel |
| `Tab` | Switch tab (help panel) |
| Arrow keys | Navigate within panel |
| `Home` / `End` | Jump to top / bottom |
| `PageUp` / `PageDown` | Scroll page |

## Mouse Support

| Action | Behaviour |
|--------|-----------|
| Left click on tab bar | Switch to clicked tab |
| Left click on columns | Start text selection |
| Mouse drag | Extend selection |
| Mouse release | Copy selected text to system clipboard |
| Wheel up | Scroll preview panel up |
| Wheel down | Scroll preview panel down |

---

See also: [Configuration Guide](CONFIGURATION.md) | [配置指南](CONFIGURATION_CN.md)
