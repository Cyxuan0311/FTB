[中文文档](PREREQUISITES_CN.md)  |  English

# Prerequisites

FTB uses external CLI tools to provide enhanced preview features. All are **optional** — if a tool is not installed, the corresponding preview type gracefully falls back or skips.

## Preview tools

| Tool | Preview type | Install |
|------|-------------|---------|
| [timg](https://github.com/hzeller/timg) | Media preview (GIF, WebP, video) | `apt install timg` |
| [ffmpeg](https://ffmpeg.org) | Video first-frame extraction (used with timg) | `apt install ffmpeg` |
| [glow](https://github.com/charmbracelet/glow) | Markdown preview / DOCX rendering | `apt install glow` |
| [xleak](https://github.com/anomalyco/xleak) | Spreadsheet preview (xlsx, xls, ods) | `cargo install xleak` |
| [eyeD3](https://eyed3.readthedocs.io) | Audio metadata display | `apt install eyed3` |
| [hygg](https://github.com/anomalyco/hygg) | PDF text extraction | `cargo install hygg` |
| [pandoc](https://pandoc.org) | DOC/DOCX to Markdown conversion | `apt install pandoc` |
| [catdoc](http://www.wagner.pp.ru/~vitus/software/catdoc/) | Old `.doc` text extraction | `apt install catdoc` |
| [xxd](https://linux.die.net/man/1/xxd) | Hex dump (part of vim-common) | (usually pre-installed) |

## Archive listing

FTB uses these tools in a tiered fallback order to list archive contents:

| Tool | Archive formats | Install |
|------|----------------|---------|
| [unzip](https://infozip.sourceforge.net) | `.zip` | `apt install unzip` |
| [tar](https://www.gnu.org/software/tar/) | `.tar`, `.tgz`, `.tbz2`, `.txz`, `.tzst` | (usually pre-installed) |
| [7z](https://www.7-zip.org) | `.7z`, `.rar`, `.cab` | `apt install p7zip-full` |
| [isoinfo](https://www.gnu.org/software/xorriso/) | `.iso` | `apt install genisoimage` |

## Other tools

| Tool | Feature | Install |
|------|---------|---------|
| [libsixel](https://github.com/libsixel/libsixel) | Native terminal image output (sixel) | `apt install libsixel-dev` |
| [qjs (QuickJS)](https://bellard.org/quickjs/) | Plugin system JavaScript runtime | `apt install quickjs` or build from source |
| [fdfind](https://github.com/sharkdp/fd) / `fd` | Faster file search in FuzzyFinder | `apt install fd-find` |

## Quick install (Ubuntu/Debian)

```bash
# Preview tools
sudo apt-get install -y timg ffmpeg glow eyed3 pandoc catdoc xxd unzip p7zip-full genisoimage

# Other
sudo apt-get install -y libsixel-dev quickjs fd-find
```

For `xleak` and `hygg` (Rust tools):
```bash
cargo install xleak hygg
```

## Shell integration (optional)

FTB can change your shell's working directory on exit. Add a wrapper function to your shell config:

**Bash** (`~/.bashrc`):
```bash
function ftb() { command FTB "$@"; [ -f ~/.cache/ftb/cwd ] && cd "$(<~/.cache/ftb/cwd)" && rm ~/.cache/ftb/cwd; }
alias FTB='ftb'
```

**Zsh** (`~/.zshrc`):
```zsh
function ftb() { command FTB "$@"; [ -f ~/.cache/ftb/cwd ] && cd "$(<~/.cache/ftb/cwd)" && rm ~/.cache/ftb/cwd; }
alias FTB='ftb'
```

Then inside FTB, press `Ctrl+B` and type `z` (or `exit` / `quit`). The program will exit and your shell will `cd` to the directory you were browsing.

Multiple terminal sessions are isolated — each shell session reads and clears its own `~/.cache/ftb/cwd` file.

---

See also: [Configuration Guide](CONFIGURATION.md) | [配置指南](CONFIGURATION_CN.md)
