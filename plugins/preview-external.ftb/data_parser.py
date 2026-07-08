#!/usr/bin/env python3
"""Data file analysis with terminal charts (CSV, TSV, JSON)."""

import sys
import os
import math

from _common import *
from _table import preview_dataframe

SPARK = "▁▂▃▄▅▆▇█"


def escape_ansi(s):
    import re
    return re.sub(r'\033\[[0-9;]*m', '', str(s))


def sparkline(values, width=30):
    if not values:
        return ""
    mn, mx = min(values), max(values)
    if mx == mn:
        mid = len(SPARK) // 2
        return SPARK[mid] * min(width, len(values))
    n = len(values)
    step = max(1, n // width) if n > width else 1
    sampled = values[::step]
    n = len(sampled)
    result = []
    for i in range(min(width, n)):
        idx = int((sampled[i] - mn) / (mx - mn) * (len(SPARK) - 1))
        idx = max(0, min(idx, len(SPARK) - 1))
        result.append(SPARK[idx])
    return "".join(result)


def distribution_chart(values, col_name, width, num_buckets=10):
    if len(values) == 0:
        return
    vals = list(values)
    mn, mx = min(vals), max(vals)
    if mx == mn:
        return
    bucket_w = (mx - mn) / num_buckets
    buckets = [0] * num_buckets
    for v in vals:
        idx = min(int((v - mn) / bucket_w), num_buckets - 1)
        buckets[idx] += 1
    max_count = max(buckets)
    if max_count == 0:
        return

    chart_w = min(30, width - 25)
    if chart_w < 10:
        chart_w = 10

    print(f"  {Y}{col_name}{R}  {D}(min={mn:.1f}  max={mx:.1f}  mean={sum(vals)/len(vals):.1f}){R}")
    print(f"  {D}  {chr(0x2502)}{R} {sparkline(vals, chart_w)}")
    print()


def cat_chart(series, col_name, width, top_n=8):
    counts = series.value_counts()
    total = counts.sum()
    print(f"  {Y}{col_name}{R}  {D}({len(counts)} unique){R}")
    for i, (val, cnt) in enumerate(counts.head(top_n).items()):
        label = truncate(val, 20)
        pct = cnt * 100.0 / total
        print(bar_chart(label, pct, width, i))
    if len(counts) > top_n:
        rest = counts.iloc[top_n:].sum()
        pct = rest * 100.0 / total
        print(bar_chart(f"... {len(counts) - top_n} more", pct, width, top_n))
    print()


# ─── JSON Tree Renderer ────────────────────────────────────────────

def json_tree(data, width, max_depth=4, max_items=20):
    """Render JSON as an indented tree with box-drawing characters."""

    def vlen(s):
        import re
        return len(re.sub(r'\033\[[0-9;]*m', '', str(s)))

    def render_node(node, prefix, connector, key, depth):
        line = prefix + connector

        if key is not None:
            line += f"{G}{key}{R}: "

        if isinstance(node, dict):
            empty = len(node) == 0
            line += f"{D}{{}}{R}" if empty else f"{D}{{{len(node)} keys}}{R}"
            print(line)
            if empty or depth >= max_depth:
                return
            items = list(node.items())
            total = len(items)
            has_more = total > max_items
            shown = min(max_items, total)
            for i in range(shown):
                k, v = items[i]
                is_last = (i == shown - 1) and not has_more
                npref = prefix + ("   " if "└" in connector else "│  ")
                render_node(v, npref, "├─ " if not is_last else "└─ ", k, depth + 1)
            if has_more:
                npref = prefix + ("   " if "└" in connector else "│  ")
                print(f"{npref}{D}└─ ... {total - shown} more keys{R}")

        elif isinstance(node, list):
            empty = len(node) == 0
            line += f"{D}[]{R}" if empty else f"{D}[{len(node)} items]{R}"
            print(line)
            if empty or depth >= max_depth:
                return
            total = len(node)
            has_more = total > max_items
            shown = min(max_items, total)
            for i in range(shown):
                item = node[i]
                is_last = (i == shown - 1) and not has_more
                npref = prefix + ("   " if "└" in connector else "│  ")
                render_node(item, npref, "├─ " if not is_last else "└─ ", str(i), depth + 1)
            if has_more:
                npref = prefix + ("   " if "└" in connector else "│  ")
                print(f"{npref}{D}└─ ... {total - shown} more items{R}")

        elif isinstance(node, bool):
            line += f"{K}{str(node).lower()}{R}"
            print(line)

        elif isinstance(node, (int, float)):
            line += f"{M}{node}{R}"
            print(line)

        elif isinstance(node, str):
            overhead = 11
            avail = width - vlen(line) - overhead
            val = truncate(str(node), max(4, avail))
            line += f"{S}\"{val}\"{R}"
            print(line)

        elif node is None:
            line += f"{K}null{R}"
            print(line)

        else:
            line += f"{W}{truncate(str(node), 60)}{R}"
            print(line)

    render_node(data, "  ", "", None, 0)


# ─── CSV / TSV ──────────────────────────────────────────────────────

def parse_csv(path, width, sep=','):
    import pandas as pd
    try:
        df = pd.read_csv(path, sep=sep, nrows=2000, low_memory=False)
    except Exception:
        return False

    fmt = "TSV" if sep == '\t' else "CSV"
    _show_dataframe(df, width, fmt)
    return True


# ─── JSON ───────────────────────────────────────────────────────────

def parse_json(path, width):
    import json
    try:
        with open(path) as f:
            data = json.load(f)
    except Exception:
        return False

    section("JSON Structure")
    json_tree(data, width)

    try:
        import pandas as pd
        if isinstance(data, list) and len(data) > 0 and isinstance(data[0], dict):
            df = pd.json_normalize(data)
            _show_dataframe(df, width, "Normalized Table")
    except Exception:
        pass

    return True


# ─── Shared DataFrame display ───────────────────────────────────────

def _show_dataframe(df, width, fmt):
    n_rows, n_cols = df.shape

    section(f"Data Summary ({fmt})")
    label_value("Rows", f"{n_rows:,}")
    label_value("Columns", str(n_cols))

    col_info = []
    num_cols = []
    cat_cols = []

    for c in df.columns:
        dtype = df[c].dtype
        non_null = df[c].count()
        pct = non_null * 100.0 / n_rows if n_rows > 0 else 100
        is_num = dtype.kind in ('i', 'u', 'f')
        n_unique = df[c].nunique()
        col_info.append((c, dtype, non_null, pct, is_num, n_unique))
        if is_num and non_null > 0:
            num_cols.append(c)
        elif not is_num and non_null > 0:
            cat_cols.append(c)

    section("Columns")
    hdr = f"  {'Name':<20} {'Non-null':>10} {'Type':<12} {'Unique':>8}"
    print(hdr)
    print(f"  {'-'*20} {'-'*10} {'-'*12} {'-'*8}")
    for c, dtype, nn, pct, is_num, nu in col_info[:25]:
        name = truncate(c, 18)
        nn_str = f"{nn:,} ({pct:.0f}%)"
        dtype_s = str(dtype)
        uniq = str(nu)
        print(f"  {G}{name:<20}{R} {nn_str:>10} {dtype_s:<12} {uniq:>8}")
    if len(col_info) > 25:
        print(f"  {D}... and {len(col_info) - 25} more columns{R}")

    if num_cols:
        section("Numeric Columns")
        for c in num_cols:
            vals = df[c].dropna()
            if len(vals) < 2:
                continue
            try:
                vals_f = vals.astype(float)
                distribution_chart(vals_f, c, width)
            except Exception:
                pass

    if cat_cols:
        section("Top Categories")
        for c in cat_cols:
            try:
                cat_chart(df[c], c, width)
            except Exception:
                pass

    section(f"Preview (first {min(n_rows, 12)} rows)")
    preview_dataframe(df, width)


# ─── Main ───────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: data_parser.py <data_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()

    print(f"\n{B}[ Data File Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))

    parsed = False

    if ext == '.csv':
        parsed = parse_csv(path, width, sep=',')
    elif ext == '.tsv':
        parsed = parse_csv(path, width, sep='\t')
    elif ext == '.json':
        parsed = parse_json(path, width)

    if not parsed:
        label_value("Type", try_file(path))
        print(f"\n  {D}(install pandas for detailed data analysis){R}")

    print()


if __name__ == "__main__":
    main()
