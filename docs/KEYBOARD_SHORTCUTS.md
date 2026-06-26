# FTB Keyboard Shortcuts

## Navigation

| Key | Action |
|-----|--------|
| `j` / `Down` | Move selection down |
| `k` / `Up` | Move selection up |
| `h` / `Left` | Go to parent directory |
| `l` / `Right` / `Enter` | Enter directory / open file |
| `Home` | Jump to first item |
| `End` | Jump to last item |
| `PageUp` | Previous page |
| `PageDown` | Next page |

## Search

| Key | Action |
|-----|--------|
| `/` | Enter search mode |
| `Escape` | Clear search / exit search mode |
| `Backspace` | Delete last character / exit if empty |
| Character keys | Type search query (in search mode) |

## File Operations

| Key | Action |
|-----|--------|
| `y` | Copy selected item |
| `x` | Cut selected item |
| `p` | Paste from clipboard |
| `Delete` / `Ctrl+D` | Delete selected item |

## Preview Panel Scrolling

| Key | Action |
|-----|--------|
| `Alt+J` | Scroll preview down |
| `Alt+K` | Scroll preview up |
| `Alt+H` | Scroll preview left |
| `Alt+L` | Scroll preview right |
| Mouse Wheel | Scroll preview up/down |

## File Opening

| Key | Action |
|-----|--------|
| `Enter` | Open file with default system program |

## Command Mode

Press `Ctrl+B` to enter prefix mode, then type a command:

| Command | Alias | Action |
|---------|-------|--------|
| `open` | `op` | Open with picker dialog |
| `openwith` | `ow` | Manual specify program |
| `opencfg` | `oc` | Configure openers |
| `theme` | `th` | Open theme selector |
| `rename` | `rn` | Rename selected item |
| `newfile` | `nf` | Create new file |
| `newfolder` | `nd` | Create new folder |
| `preview` | `p` | Full file preview |
| `details` | `d` | Folder details |
| `jump` | `j` | Jump to directory |
| `vim` | `v` | Open in Vim editor |
| `search` | `s` | Enter search mode |
| `calendar` | `cal` | Calendar panel |
| `layout` | `lo` | Layout settings |
| `help` | `h` | Help panel |
| `ssh` | - | SSH connection (if enabled) |

Press `Enter` to execute, `Escape` to cancel.

## Application Control

| Key | Action |
|-----|--------|
| `Ctrl+C` | Quit FTB |
| `Ctrl+R` | Reload configuration file |
| `Escape` | Close active panel / clear search |

## Vim Editor

When the Vim editor is open (via `:vim` command), the following keys are available:

### Normal Mode

| Key | Action |
|-----|--------|
| `h` / `j` / `k` / `l` | Move cursor |
| `w` | Next word |
| `b` | Previous word |
| `0` | Beginning of line |
| `$` | End of line |
| `gg` | Beginning of file |
| `G` | End of file |
| `PageUp` / `PageDown` | Scroll page |
| `i` | Insert mode |
| `a` | Append mode |
| `o` | New line below |
| `O` | New line above |
| `x` | Delete character |
| `dd` | Delete line |
| `yy` | Yank line |
| `p` / `P` | Paste |
| `u` | Undo |
| `Ctrl+R` | Redo |

### Command Mode (in Vim editor)

| Command | Action |
|---------|--------|
| `:w` | Save file |
| `:q` | Quit editor |
| `:wq` | Save and quit |
| `:q!` | Quit without saving |

### Search and Replace

| Key | Action |
|-----|--------|
| `/pattern` | Search forward |
| `:s/old/new` | Replace first occurrence |
| `:%s/old/new/g` | Replace all occurrences |

### Markdown Preview

| Key | Action |
|-----|--------|
| `Ctrl+M` | Toggle Markdown preview mode |

## Panel Navigation

When a panel (theme, help, calendar, etc.) is open:

| Key | Action |
|-----|--------|
| `Escape` | Close panel |
| `Tab` | Navigate between elements (in some panels) |
| `Enter` | Confirm / select |
| Arrow keys | Navigate within panel |

---

See also: [Configuration Guide](CONFIGURATION.md)
