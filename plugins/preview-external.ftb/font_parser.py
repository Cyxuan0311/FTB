#!/usr/bin/env python3
"""Font file analysis (TTF, OTF, WOFF, WOFF2, TTC)."""

import sys
import os
from io import BytesIO

from _common import *

WEIGHT_MAP = {
    100: 'Thin', 200: 'ExtraLight', 300: 'Light',
    350: 'SemiLight',
    400: 'Normal (Regular)',
    500: 'Medium', 600: 'SemiBold', 700: 'Bold',
    800: 'ExtraBold', 900: 'Black',
    950: 'ExtraBlack',
}

WIDTH_MAP = {
    1: 'UltraCondensed', 2: 'ExtraCondensed', 3: 'Condensed',
    4: 'SemiCondensed', 5: 'Medium (normal)',
    6: 'SemiExpanded', 7: 'Expanded',
    8: 'ExtraExpanded', 9: 'UltraExpanded',
}

NAME_IDS = {
    0: 'Copyright', 1: 'Family', 2: 'Subfamily (Style)',
    3: 'Unique ID', 4: 'Full Name', 5: 'Version',
    6: 'PostScript Name', 8: 'Foundry',
    9: 'Designer', 10: 'Description', 11: 'Vendor URL',
    12: 'Designer URL', 13: 'License', 14: 'License URL',
    16: 'Typographic Family', 17: 'Typographic Subfamily',
}

PRIORITY_NAMES = [16, 1, 17, 2, 5, 8, 9, 11, 12, 13, 14, 0, 4]


def best_name(name_table, name_id):
    for rec in name_table.names:
        if rec.nameID == name_id and rec.platformID == 3 and rec.langID == 1033:
            try:
                return rec.toUnicode()
            except Exception:
                pass
    for rec in name_table.names:
        if rec.nameID == name_id and rec.platformID == 1 and rec.langID == 0:
            try:
                return rec.toUnicode()
            except Exception:
                pass
    for rec in name_table.names:
        if rec.nameID == name_id:
            try:
                return rec.toUnicode()
            except Exception:
                pass
    return None


def parse_single_font(path_or_file, width, label=None):
    from fontTools.ttLib import TTFont
    font = TTFont(path_or_file, fontNumber=-1)

    sf_version = font.sfntVersion
    if isinstance(sf_version, str):
        sf_bytes = sf_version.encode('latin-1') if len(sf_version) == 4 else b'\x00\x01\x00\x00'
        sf_int = int.from_bytes(sf_bytes, 'big')
    else:
        sf_int = sf_version

    if sf_int == 0x00010000 or sf_int == 0x74727565:
        fmt = "TrueType"
    elif sf_int == 0x4F54544F:
        fmt = "OpenType (CFF)"
    elif sf_int == 0x774F4646:
        fmt = "WOFF"
    elif sf_int == 0x774F4632:
        fmt = "WOFF2"
    else:
        fmt = f"Unknown (0x{sf_int:08x})"

    if label:
        section(label)
    else:
        section("General")

    name = font.get('name')

    if name:
        for nid in PRIORITY_NAMES:
            val = best_name(name, nid)
            if val:
                lname = NAME_IDS.get(nid, f"NameID {nid}")
                if nid in (0, 10, 13) and len(val) > 120:
                    val = val[:120] + "..."
                label_value(lname, val)

    label_value("Format", fmt)

    section("Metrics")
    head = font.get('head')
    maxp = font.get('maxp')
    os2 = font.get('OS/2')
    post = font.get('post')

    if maxp:
        label_value("Glyphs", f"{maxp.numGlyphs:,}")

    if head:
        label_value("Units/Em", str(head.unitsPerEm))
        label_value("Mac Style",
            ", ".join(s for s, b in [('Bold',1),('Italic',2),('Underline',4),('Outline',8),('Shadow',16),('Condensed',32),('Extended',64)]
                      if head.macStyle & b) or "Normal")

    if os2:
        w = os2.usWeightClass
        wl = WEIGHT_MAP.get(w, '')
        label_value("Weight", f"{w} ({wl})" if wl else str(w))
        w = os2.usWidthClass
        wl = WIDTH_MAP.get(w, '')
        label_value("Width", f"{w} ({wl})" if wl else str(w))

    if post:
        angle = post.italicAngle
        label_value("Italic Angle", f"{angle:.1f}°{' (italic)' if abs(angle) > 0.01 else ''}")
        if post.isFixedPitch:
            label_value("Monospace", "Yes")

    section("Tables")
    tables = list(font.keys())
    display_tables = [t for t in tables if not t.startswith('_') and t not in ('GlyphOrder',)]
    label_value("Count", str(len(display_tables)))
    essentials = [t for t in display_tables if t in ('head','hhea','maxp','OS/2','name','post','cmap','hmtx')]
    outlines = [t for t in display_tables if t in ('glyf','loca','CFF ','CFF2','sbix','CBDT')]
    layout = [t for t in display_tables if t in ('GPOS','GSUB','GDEF','kern','bsln','BASE')]
    other = [t for t in display_tables if t not in essentials+outlines+layout]
    for group, label_name in [(essentials, 'Essential'), (outlines, 'Outlines'), (layout, 'Layout'), (other, 'Other')]:
        if group:
            print(f"  {D}{label_name}: {', '.join(group[:8])}{'...' if len(group) > 8 else ''}{R}")

    if 'fvar' in font:
        fvar = font['fvar']
        section("Variable Axes")
        axes = fvar.axes
        if axes:
            for ax in axes:
                fmt_ax = f"{ax.axisTag}:  {ax.minValue:.0f}..{ax.maxValue:.0f} (Default {ax.defaultValue:.0f})"
                print(f"  {C}{fmt_ax}{R}")
            for inst in fvar.instances:
                coords = ", ".join(f"{a}={v:.0f}" for a, v in inst.coordinates.items())
                name_str = inst.postScriptName or ''
                if name_str:
                    print(f"    {W}  Instance: {name_str} [{coords}]{R}")
                else:
                    print(f"    {W}  Instance: [{coords}]{R}")

    font.close()
    return True


def parse_ttc(path, width):
    try:
        from fontTools.ttLib import TTCollection
        tc = TTCollection(path)
        count = len(tc.fonts)
        label_value("Fonts in Collection", str(count))
        for i in range(min(count, 5)):
            font = tc.fonts[i]
            name = font.get('name')
            family = best_name(name, 16) or best_name(name, 1) or '?'
            sub = best_name(name, 17) or best_name(name, 2) or ''
            print(f"  {C}[{i}]{R} {family} {D}{sub}{R}")
        if count > 5:
            print(f"  {W}... and {count - 5} more fonts{R}")
    except ImportError:
        return parse_single_font(path, width)
    except Exception:
        return False
    return True


def probe_format(path):
    with open(path, 'rb') as f:
        magic = f.read(4)
    if magic == b'ttcf':
        return 'ttc'
    if magic in (b'\x00\x01\x00\x00', b'true', b'OTTO'):
        return 'otf'
    if magic == b'wOFF':
        return 'woff'
    if magic == b'wOF2':
        return 'woff2'
    return None


def main():
    if len(sys.argv) < 2:
        print("Usage: font_parser.py <font_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    print(f"\n{B}[ Font Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))

    parsed = False

    fmt = probe_format(path)
    if fmt == 'ttc':
        parsed = parse_ttc(path, width)
    else:
        try:
            parsed = parse_single_font(path, width)
        except Exception:
            parsed = False

    if not parsed:
        import subprocess
        try:
            r = subprocess.run(['file', '-Lb', path], capture_output=True, text=True, timeout=5)
            label_value("Type", r.stdout.strip() if r.returncode == 0 else "unknown")
        except Exception:
            label_value("Type", "unknown")

        with open(path, 'rb') as fh:
            raw = fh.read(64)
        print()
        section("File Header Hex")
        hex_dump(raw, 0, width)
        print()

    print()


if __name__ == "__main__":
    main()
