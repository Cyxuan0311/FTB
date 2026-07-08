"""Terminal table renderer with ANSI formatting."""

import math
from _common import *


def render_table(headers, rows, width, col_align=None):
    if not headers:
        return

    ncols = len(headers)
    if col_align is None:
        col_align = ['<'] * ncols

    col_widths = []
    for i in range(ncols):
        hdr_len = len(str(headers[i]))
        max_content = max((len(str(r[i])) for r in rows), default=0) if rows else 0
        col_widths.append(max(hdr_len, max_content, 4))

    sep_text = f" {D}\u2502{R} "
    sep_len = len(f" {D}\u2502{R} ") - 8
    total = sum(col_widths) + (ncols - 1) * 3
    slack = width - total - 4
    if slack > 0:
        add_per_col = slack // ncols
        rem = slack % ncols
        for i in range(ncols):
            col_widths[i] += add_per_col
            if i < rem:
                col_widths[i] += 1

    max_avail = (width - 4 - (ncols - 1) * 3) // ncols
    if max_avail < 4:
        max_avail = 4
    col_widths = [min(w, max_avail) for w in col_widths]

    def fmt_cell(val, w, align):
        s = truncate(str(val), w)
        if align == '>':
            return s.rjust(w)
        return s.ljust(w)

    sep = f" {D}\u2502{R} "

    hdr_cells = [f"{B}{fmt_cell(headers[i], col_widths[i], col_align[i])}{R}" for i in range(ncols)]
    print(f"  {hdr_cells[0]}", end="")
    for i in range(1, ncols):
        print(f"{sep}{hdr_cells[i]}", end="")
    print()

    total_w = sum(col_widths) + (ncols - 1) * 3
    hbar = '\u2500'
    print(f"  {D}{hbar * total_w}{R}")

    for row in rows:
        cells = [f"{W}{fmt_cell(row[i], col_widths[i], col_align[i])}{R}" for i in range(ncols)]
        print(f"  {cells[0]}", end="")
        for i in range(1, ncols):
            print(f"{sep}{cells[i]}", end="")
        print()


def preview_dataframe(df, width, max_rows=12):
    if df.empty:
        return
    cols = list(df.columns)
    ncols = len(cols)
    max_col_w = max(10, width // ncols - 2)
    if max_col_w < 6:
        max_col_w = 6

    rows_to_show = min(max_rows, len(df))

    headers = [truncate(c, max_col_w) for c in cols]

    col_align = []
    for c in cols:
        kind = df[c].dtype.kind
        col_align.append('>' if kind in ('i', 'u', 'f') else '<')

    data_rows = []
    for i in range(rows_to_show):
        row = df.iloc[i]
        cells = []
        for c in cols:
            v = row[c]
            s = str(v) if not (isinstance(v, float) and math.isnan(v)) else ""
            cells.append(truncate(s, max_col_w))
        data_rows.append(cells)

    render_table(headers, data_rows, width, col_align)

    if len(df) > rows_to_show:
        print(f"  {D}... and {len(df) - rows_to_show} more rows{R}")
