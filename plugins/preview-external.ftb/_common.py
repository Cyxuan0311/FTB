"""Shared utilities for FTB preview parsers."""

import os

_PLUGIN_DIR = os.path.dirname(os.path.abspath(__file__))

THEMES = {
    "default": {
        "R": "\033[0m",
        "B": "\033[1;34m",
        "Y": "\033[1;33m",
        "G": "\033[0;32m",
        "C": "\033[0;36m",
        "W": "\033[0;37m",
        "D": "\033[2m",
        "S": "\033[1;33m",
        "M": "\033[0;35m",
        "K": "\033[0;36m",
        "CAT": [
            "\033[38;5;39m", "\033[38;5;214m", "\033[38;5;118m",
            "\033[38;5;205m", "\033[38;5;99m", "\033[38;5;43m",
            "\033[38;5;227m", "\033[38;5;203m",
        ],
        "BAR_FULL": "█", "BAR_EMPTY": "░",
    },
    "monochrome": {
        "R": "", "B": "", "Y": "", "G": "", "C": "", "W": "", "D": "",
        "S": "", "M": "", "K": "",
        "CAT": [""] * 8,
        "BAR_FULL": "#", "BAR_EMPTY": ".",
    },
    "highcontrast": {
        "R": "\033[0m",
        "B": "\033[1;94m",
        "Y": "\033[1;93m",
        "G": "\033[1;92m",
        "C": "\033[1;96m",
        "W": "\033[1;97m",
        "D": "\033[1;90m",
        "S": "\033[1;93m",
        "M": "\033[1;95m",
        "K": "\033[1;96m",
        "CAT": [
            "\033[1;91m", "\033[1;93m", "\033[1;92m",
            "\033[1;94m", "\033[1;95m", "\033[1;96m",
            "\033[1;97m", "\033[1;91m",
        ],
        "BAR_FULL": "█", "BAR_EMPTY": "░",
    },
    "nord": {
        "R": "\033[0m",
        "B": "\033[38;5;67m",
        "Y": "\033[38;5;150m",
        "G": "\033[38;5;108m",
        "C": "\033[38;5;110m",
        "W": "\033[38;5;188m",
        "D": "\033[38;5;242m",
        "S": "\033[38;5;180m",
        "M": "\033[38;5;167m",
        "K": "\033[38;5;139m",
        "CAT": [
            "\033[38;5;167m", "\033[38;5;180m", "\033[38;5;150m",
            "\033[38;5;108m", "\033[38;5;110m", "\033[38;5;67m",
            "\033[38;5;139m", "\033[38;5;167m",
        ],
        "BAR_FULL": "█", "BAR_EMPTY": "░",
    },
    "dracula": {
        "R": "\033[0m",
        "B": "\033[38;5;141m",
        "Y": "\033[38;5;228m",
        "G": "\033[38;5;84m",
        "C": "\033[38;5;81m",
        "W": "\033[38;5;254m",
        "D": "\033[38;5;245m",
        "S": "\033[38;5;228m",
        "M": "\033[38;5;84m",
        "K": "\033[38;5;141m",
        "CAT": [
            "\033[38;5;210m", "\033[38;5;228m", "\033[38;5;84m",
            "\033[38;5;81m", "\033[38;5;141m", "\033[38;5;213m",
            "\033[38;5;229m", "\033[38;5;210m",
        ],
        "BAR_FULL": "█", "BAR_EMPTY": "░",
    },
    "gruvbox": {
        "R": "\033[0m",
        "B": "\033[38;5;214m",
        "Y": "\033[38;5;142m",
        "G": "\033[38;5;107m",
        "C": "\033[38;5;109m",
        "W": "\033[38;5;223m",
        "D": "\033[38;5;246m",
        "S": "\033[38;5;214m",
        "M": "\033[38;5;173m",
        "K": "\033[38;5;109m",
        "CAT": [
            "\033[38;5;167m", "\033[38;5;214m", "\033[38;5;142m",
            "\033[38;5;107m", "\033[38;5;109m", "\033[38;5;66m",
            "\033[38;5;173m", "\033[38;5;167m",
        ],
        "BAR_FULL": "█", "BAR_EMPTY": "░",
    },
}

def _load_theme():
    fpath = os.path.join(_PLUGIN_DIR, ".color_theme")
    name = "default"
    if os.path.exists(fpath):
        try:
            name = open(fpath).read().strip().lower()
        except Exception:
            pass
    if name not in THEMES:
        name = "default"
    t = THEMES[name]
    return name, t

_theme_name, _theme = _load_theme()
R = _theme["R"]
B = _theme["B"]
Y = _theme["Y"]
G = _theme["G"]
C = _theme["C"]
W = _theme["W"]
D = _theme["D"]
S = _theme["S"]
M = _theme["M"]
K = _theme["K"]
COLORS_CAT = _theme["CAT"]
BAR_FULL = _theme["BAR_FULL"]
BAR_EMPTY = _theme["BAR_EMPTY"]


def section(title):
    print(f"\n{B}>> {title} {R}")


def label_value(label, value, color=G):
    print(f"  {Y}{label}{R}: {color}{value}{R}")


def fmt_size(n):
    if n >= 1024 ** 3:
        return f"{n / 1024**3:.1f} GiB"
    if n >= 1024 * 1024:
        return f"{n:,} ({n / 1024 / 1024:.1f} MiB)"
    elif n >= 1024:
        return f"{n:,} ({n / 1024:.1f} KiB)"
    return f"{n:,} B"


def hex_dump(data, offset=0, width=80):
    bytes_per_line = 8 if width < 80 else 16
    sep = "  "
    ascii_start = 11 + 3 + bytes_per_line * 3 + (1 if bytes_per_line > 8 else 0)
    if ascii_start + bytes_per_line + 2 > width:
        ascii_start = 0
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        addr = offset + i
        hex_part = " ".join(f"{b:02x}" for b in chunk)
        if bytes_per_line > 8:
            hex_part = hex_part[:23] + " " + hex_part[23:]
        line = f"{C}{addr:08x}{R}{sep}{hex_part:<{bytes_per_line * 3 + 1}}"
        if ascii_start:
            asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
            line += f" |{asc}|"
        print(line)


def bar_chart(label, pct, max_w, ci=0):
    bar_w = max_w - len(str(label)) - 20
    if bar_w < 5:
        bar_w = 5
    filled = max(1, int(pct * bar_w / 100))
    empty = bar_w - filled
    color = COLORS_CAT[ci % len(COLORS_CAT)]
    bar = color + BAR_FULL * filled + D + BAR_EMPTY * empty + R
    return f"  {label:<{len(str(label))}} {bar} {G}{pct:5.1f}%{R}"


def try_file(path):
    import subprocess
    try:
        r = subprocess.run(['file', '-b', path], capture_output=True, text=True, timeout=5)
        return r.stdout.strip() if r.returncode == 0 else "unknown"
    except Exception:
        return "unknown"


def truncate(s, max_w):
    s = str(s)
    if len(s) <= max_w:
        return s
    if max_w < 4:
        return s[:max_w]
    return s[:max_w - 3] + "..."
