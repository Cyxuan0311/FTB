#!/usr/bin/env python3
"""Archive analysis (APK, RAR, UnityPackage)."""

import sys
import os
import struct
import re
import tarfile
import zipfile
from io import BytesIO

from _common import *


# ===== APK specific =================================================

def debug_stderr(tag, msg):
    print(f"[apk_parser:{tag}] {msg}", file=sys.stderr, flush=True)


def analyze_zip_header(f):
    f.seek(0)
    raw = f.read(30)
    if len(raw) < 30:
        print(f"  {R}File too small for ZIP header{R}")
        return
    magic, ver, flags, method, modtime, moddate, crc, comp_size, uncomp_size, fname_len, extra_len = \
        struct.unpack("<IHHHHHIIIHH", raw)
    debug_stderr("analyze_zip_header", f"magic=0x{magic:08x} ver={ver} method={method}")
    label_value("Magic",
        f"50 4B 03 04 -> {chr(magic & 0xFF)}{chr((magic>>8) & 0xFF)} (PK\\x03\\x04)")
    label_value("Version needed", f"{ver>>8}.{ver&0xff}")
    label_value("Compression",
        f"method={method}  ({'stored' if method==0 else 'deflated' if method==8 else method})")
    label_value("Flags", f"0x{flags:04x}  {'(encrypted)' if flags&1 else ''}")
    label_value("CRC-32", f"0x{crc:08x}")
    label_value("Compressed", fmt_size(comp_size))
    label_value("Uncompressed", fmt_size(uncomp_size))


def analyze_central_directory(f):
    try:
        zf = zipfile.ZipFile(f)
    except zipfile.BadZipfile:
        print(f"  {R}Not a valid ZIP/APK file{R}")
        return None, None
    infos = zf.infolist()
    total = len(infos)
    count = min(total, 25)
    debug_stderr("analyze_central_directory", f"{total} total entries, showing {count}")
    section(f"Central Directory ({count} of {total} entries)")
    print(f"  {B}{'Offset':>10} {'Compressed':>12} {'Uncompressed':>12} {'Ratio':>6}  Name{R}")
    print(f"  {D}{'-'*10} {'-'*12} {'-'*12} {'-'*6}  {'-'*40}{R}")
    for i in range(count):
        zi = infos[i]
        ratio = "N/A"
        if zi.compress_size > 0 and zi.file_size > 0:
            ratio = f"{zi.compress_size * 100 // zi.file_size}%"
        print(f"  {C}{zi.header_offset:08x}{R}  {G}{zi.compress_size:>10,}{R}  "
              f"{G}{zi.file_size:>10,}{R}  {Y}{ratio:>5}{R}  {W}{zi.filename}{R}")
    if total > count:
        print(f"  {D}... and {total - count} more entries{R}")
    return zf, infos


def analyze_dex(f, zf, infos):
    dex_entry = None
    for zi in infos:
        if zi.filename == "classes.dex":
            dex_entry = zi
            break
    if not dex_entry:
        print(f"  {D}(no classes.dex found){R}")
        return
    section("DEX Header (classes.dex)")
    label_value("Offset in ZIP", f"0x{dex_entry.header_offset:x}")
    label_value("Compressed", fmt_size(dex_entry.compress_size))
    label_value("Uncompressed", fmt_size(dex_entry.file_size))
    raw = zf.read("classes.dex")
    debug_stderr("analyze_dex", f"classes.dex size={len(raw)} bytes")
    if len(raw) < 0x70:
        print(f"  {R}DEX too small: {len(raw)} bytes{R}")
        return
    magic = raw[:8]
    checksum = struct.unpack("<I", raw[8:12])[0]
    signature = raw[12:32]
    file_size = struct.unpack("<I", raw[32:36])[0]
    header_size = struct.unpack("<I", raw[36:40])[0]
    endian_tag = struct.unpack("<I", raw[40:44])[0]
    link_size = struct.unpack("<I", raw[44:48])[0]
    link_off = struct.unpack("<I", raw[48:52])[0]
    map_off = struct.unpack("<I", raw[52:56])[0]
    string_ids_size = struct.unpack("<I", raw[56:60])[0]
    string_ids_off = struct.unpack("<I", raw[60:64])[0]
    type_ids_size = struct.unpack("<I", raw[64:68])[0]
    type_ids_off = struct.unpack("<I", raw[68:72])[0]
    proto_ids_size = struct.unpack("<I", raw[72:76])[0]
    proto_ids_off = struct.unpack("<I", raw[76:80])[0]
    field_ids_size = struct.unpack("<I", raw[80:84])[0]
    field_ids_off = struct.unpack("<I", raw[84:88])[0]
    method_ids_size = struct.unpack("<I", raw[88:92])[0]
    method_ids_off = struct.unpack("<I", raw[92:96])[0]
    class_defs_size = struct.unpack("<I", raw[96:100])[0]
    class_defs_off = struct.unpack("<I", raw[100:104])[0]
    data_size = struct.unpack("<I", raw[104:108])[0]
    data_off = struct.unpack("<I", raw[108:112])[0]
    magic_str = " ".join(f"{b:02x}" for b in magic[:4]) + "  " + \
                "".join(chr(b) if 32 <= b < 127 else "." for b in magic[4:])
    sig_short = ":".join(f"{b:02x}" for b in signature[:8])
    endian_str = ("little-endian" if endian_tag == 0x12345678 else
                  "big-endian" if endian_tag == 0x78563412 else
                  f"unknown (0x{endian_tag:08x})")
    endian_good = endian_tag == 0x12345678
    dex_ver = "".join(chr(b) if 32 <= b < 127 else "" for b in magic[4:8]).rstrip("\x00")
    endian_mark = "[OK]" if endian_good else "[FAIL]"
    label_value("Magic", f"{magic_str}")
    label_value("DEX version", f"{dex_ver}")
    label_value("Checksum", f"0x{checksum:08x}")
    label_value("SHA-1 Signature", f"{sig_short}...")
    label_value("File Size", fmt_size(file_size))
    label_value("Header Size", f"{header_size} bytes")
    label_value("Endian Tag", f"0x{endian_tag:08x} ({endian_str}) {endian_mark}")
    label_value("Map Offset", f"0x{map_off:x}")
    label_value("String IDs", f"{string_ids_size} entries @ 0x{string_ids_off:x}")
    label_value("Type IDs", f"{type_ids_size} entries @ 0x{type_ids_off:x}")
    label_value("Proto IDs", f"{proto_ids_size} entries @ 0x{proto_ids_off:x}")
    label_value("Field IDs", f"{field_ids_size} entries @ 0x{field_ids_off:x}")
    label_value("Method IDs", f"{method_ids_size} entries @ 0x{method_ids_off:x}")
    label_value("Class Defs", f"{class_defs_size} entries @ 0x{class_defs_off:x}")
    section("DEX Header Hex Dump (first 64 bytes)")
    hex_dump(raw[:64], offset=0)


def analyze_manifest_approx(f, zf, infos):
    for zi in infos:
        if zi.filename == "AndroidManifest.xml":
            raw = zf.read(zi)
            section("AndroidManifest.xml")
            label_value("Entry offset", f"0x{zi.header_offset:x}")
            label_value("Entry size", fmt_size(zi.file_size))
            label_value("Compressed", fmt_size(zi.compress_size))
            if zi.file_size > 0 and len(raw) >= 8:
                magic = struct.unpack("<I", raw[:4])[0]
                label_value("Binary XML magic",
                    f"0x{magic:08x}" +
                    ("  (0x00080003 = AXML header)" if magic == 0x00080003 else ""))
                hex_dump(raw[:min(32, len(raw))], offset=0)
            return
        print(f"  {D}(no AndroidManifest.xml found){R}")


def hex_dump_file_header(f, offset, size=64):
    section(f"Hex Dump (first {size} bytes)")
    f.seek(0)
    data = f.read(size)
    hex_dump(data, offset=0, width=80)
    print()


def parse_apk(path, width):
    print(f"\n{B}[ APK Binary Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))
    try:
        with open(path, "rb") as f:
            analyze_zip_header(f)
            f.seek(0)
            zf, infos = analyze_central_directory(f)
            if zf and infos is not None:
                analyze_dex(f, zf, infos)
                analyze_manifest_approx(f, zf, infos)
            section("Raw File Header (hex)")
            f.seek(0)
            raw_header = f.read(64)
            hex_dump(raw_header, 0, width)
            print()
        debug_stderr("done", "APK analysis completed successfully")
        return True
    except Exception as e:
        debug_stderr("error", str(e))
        return False


# ===== RAR specific =================================================

def fmt_ratio(compressed, original):
    if original > 0 and compressed > 0:
        return f"{compressed * 100 // original}%"
    return "N/A"


def try_rarfile(path, width):
    try:
        import rarfile
        rf = rarfile.RarFile(path)
    except ImportError:
        return None, 0
    except Exception:
        return None, 0
    infos = rf.infolist()
    total = len(infos)
    count = min(total, 50)
    label_value("Archive type", "RAR")
    label_value("Entries", str(total))
    if count > 0:
        section(f"Contents ({count} of {total} entries)")
        print(f"  {B}{'Compressed':>12} {'Uncompressed':>12} {'Ratio':>6}  Name{R}")
        print(f"  {D}{'-'*12} {'-'*12} {'-'*6}  {'-'*50}{R}")
        for i in range(count):
            zi = infos[i]
            name = zi.filename
            if zi.is_dir():
                name += "/"
            ratio = fmt_ratio(zi.compress_size, zi.file_size)
            print(f"  {G}{zi.compress_size:>10,}{R}  {G}{zi.file_size:>10,}{R}  {Y}{ratio:>5}{R}  {W}{name}{R}")
        if total > count:
            print(f"  {D}... and {total - count} more entries{R}")
    return rf, total


def try_unrar(path, width):
    try:
        import subprocess
        r = subprocess.run(['unrar', 'vt', path], capture_output=True, text=True, timeout=15)
        if r.returncode != 0:
            r = subprocess.run(['unrar', 'l', path], capture_output=True, text=True, timeout=15)
        if r.returncode == 0 and r.stdout.strip():
            print(f"  {D}(listing from unrar command){R}")
            lines = r.stdout.split('\n')
            shown = 0
            for line in lines:
                if shown >= 50:
                    remaining = len(lines) - lines.index(line)
                    print(f"  {D}... and {remaining} more lines{R}")
                    break
                print(f"  {line}")
                shown += 1
            return True
    except Exception:
        pass
    return False


def parse_rar(path, width):
    print(f"\n{B}[ RAR Archive Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))
    rf, total = try_rarfile(path, width)
    if rf is None:
        if try_unrar(path, width):
            return True
        return False
    return True


# ===== UnityPackage specific ========================================

IMPORTER_PATTERN = re.compile(r'^\s+(\w+Importer):', re.MULTILINE)
GUID_PATTERN = re.compile(r'^[0-9a-f]{32}$')

IMPORTER_LABELS = {
    'ModelImporter': 'Model',
    'TextureImporter': 'Texture',
    'ShaderImporter': 'Shader',
    'ScriptedImporter': 'Script',
    'MonoImporter': 'Script',
    'AssemblyDefinitionImporter': 'Assembly',
    'VideoImporter': 'Video',
    'AudioImporter': 'Audio',
    'FontImporter': 'Font',
    'TrueTypeFontImporter': 'Font',
    'MovieImporter': 'Video',
    'SpeedTreeImporter': 'SpeedTree',
    'PrefabImporter': 'Prefab',
    'AnimatorOverrideControllerImporter': 'Animation',
    'AnimatorStateTransitionImporter': 'Animation',
    'BlendTreeImporter': 'Animation',
    'LightingImporter': 'Lighting',
    'LightmapParameters': 'Lighting',
    'SpriteAtlasImporter': 'Sprite',
    'IHVImageFormatImporter': 'Texture',
    'PluginImporter': 'Plugin',
    'FBXImporter': 'Model',
    'NativeFormatImporter': 'Asset',
    'DefaultImporter': 'Asset',
}

EXT_MAP = {
    '.fbx': 'Model', '.obj': 'Model', '.blend': 'Model',
    '.dae': 'Model', '.3ds': 'Model', '.dxf': 'Model',
    '.max': 'Model', '.mb': 'Model', '.ma': 'Model',
    '.png': 'Texture', '.jpg': 'Texture', '.jpeg': 'Texture',
    '.tga': 'Texture', '.psd': 'Texture', '.tiff': 'Texture',
    '.exr': 'Texture', '.hdr': 'Texture', '.gif': 'Texture',
    '.bmp': 'Texture', '.svg': 'Texture',
    '.cs': 'Script', '.dll': 'Plugin', '.asmdef': 'Assembly',
    '.shader': 'Shader', '.shadergraph': 'Shader',
    '.compute': 'Shader',
    '.wav': 'Audio', '.mp3': 'Audio', '.ogg': 'Audio',
    '.aiff': 'Audio', '.flac': 'Audio',
    '.mp4': 'Video', '.mov': 'Video', '.avi': 'Video',
    '.png.jpg:': 'Texture', '.ttf': 'Font', '.otf': 'Font',
    '.prefab': 'Prefab',
    '.anim': 'Animation', '.controller': 'Animation',
    '.mat': 'Material', '.physicmaterial': 'Material',
    '.spriteatlas': 'Sprite',
    '.unity': 'Scene', '.scene': 'Scene',
    '.asset': 'Asset',
    '.json': 'Asset', '.xml': 'Asset', '.bytes': 'Asset',
    '.txt': 'Asset', '.md': 'Asset',
    '.pdf': 'Document',
}


def guess_type_from_path(path):
    return EXT_MAP.get(os.path.splitext(path)[1].lower())


def guess_static_path(meta_text):
    m = IMPORTER_PATTERN.search(meta_text)
    if m:
        return IMPORTER_LABELS.get(m.group(1))
    return None


def parse_unity(path, width):
    print(f"\n{B}[ UnityPackage Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))

    file_size = os.path.getsize(path)
    try:
        tar = tarfile.open(path, 'r|gz')
    except Exception as e:
        label_value("Error", f"Not a valid gzipped tar: {e}", color=R)
        return False

    total_uncompressed = 0
    total_compressed = file_size
    guid_dirs = {}
    current_guid = None
    MAX_ASSETS_SHOW = 30
    asset_list = []
    type_counts = {}

    try:
        for member in tar:
            total_uncompressed += member.size
            name = member.name
            parts = name.split('/', 1)
            top_dir = parts[0] if len(parts) > 0 else name
            if len(parts) == 1 and GUID_PATTERN.match(top_dir):
                current_guid = top_dir
                if current_guid not in guid_dirs:
                    guid_dirs[current_guid] = {}
            elif len(parts) == 2 and GUID_PATTERN.match(parts[0]):
                current_guid = parts[0]
                fname = parts[1]
                if current_guid not in guid_dirs:
                    guid_dirs[current_guid] = {}
                guid_dirs[current_guid][fname] = True
                if member.isfile() and fname in ('pathname', 'asset.meta', 'asset') and len(asset_list) < MAX_ASSETS_SHOW:
                    try:
                        content = tar.extractfile(member)
                        if content:
                            text = content.read().decode('utf-8', errors='replace').strip()
                            if fname == 'pathname' and text:
                                if current_guid not in guid_dirs:
                                    guid_dirs[current_guid] = {}
                                guid_dirs[current_guid]['_path'] = text
                            elif fname == 'asset.meta':
                                if '_path' not in guid_dirs.get(current_guid, {}):
                                    atype = guess_static_path(text)
                                    if atype:
                                        if current_guid not in guid_dirs:
                                            guid_dirs[current_guid] = {}
                                        guid_dirs[current_guid]['_type'] = atype
                    except Exception:
                        pass

        for guid, info in guid_dirs.items():
            pathname = info.get('_path', '')
            atype = info.get('_type', '')
            if pathname:
                path_type = guess_type_from_path(pathname)
                if path_type:
                    atype = path_type
            if not atype and not pathname:
                atype = 'Unknown'
            asset_list.append((guid, pathname, atype))
            if atype:
                type_counts[atype] = type_counts.get(atype, 0) + 1
    except Exception as e:
        print(f"  {R}Error reading archive: {e}{R}")
    finally:
        tar.close()

    total_assets = len(guid_dirs)

    section("Overview")
    label_value("Total Assets", str(total_assets))
    label_value("Compressed Size", fmt_size(total_compressed))
    label_value("Uncompressed Size", fmt_size(total_uncompressed))
    if total_uncompressed > 0 and total_compressed > 0:
        ratio = total_compressed * 100 // total_uncompressed
        label_value("Compression Ratio", f"{ratio}%")

    if type_counts:
        section("Asset Types")
        sorted_types = sorted(type_counts.items(), key=lambda x: -x[1])
        max_label_w = max(len(t) for t, _ in sorted_types)
        chart_w = min(30, width - max_label_w - 20)
        if chart_w < 10:
            chart_w = 10
        for i, (t, cnt) in enumerate(sorted_types):
            pct = cnt * 100.0 / total_assets
            filled = max(1, int(pct * chart_w / 100))
            empty = chart_w - filled
            color = COLORS_CAT[i % len(COLORS_CAT)]
            bar = color + "█" * filled + D + "░" * empty + R
            print(f"  {t:<{max_label_w}} {bar} {G}{cnt:>4}{R} ({pct:.0f}%)")

    if asset_list:
        section(f"Assets (first {len(asset_list)} of {total_assets})")
        max_path_w = width - 30
        if max_path_w < 20:
            max_path_w = 20
        print(f"  {B}{'Type':<15} {'Path':<{max_path_w}}{R}")
        print(f"  {D}{'─'*15} {'─'*max_path_w}{R}")
        for guid, pathname, atype in asset_list:
            disp = pathname if pathname else guid[:12] + "..."
            disp = disp[:max_path_w - 3] + "..." if len(disp) > max_path_w else disp
            print(f"  {C}{atype:<15}{R} {W}{disp}{R}")

    if total_assets > len(asset_list):
        remaining = total_assets - len(asset_list)
        print(f"\n  {D}... and {remaining} more assets (use --width to show more){R}")

    print()
    return True


# ===== Main dispatch ================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: archive_parser.py <archive_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()
    parsed = False

    if ext == '.apk':
        parsed = parse_apk(path, width)
    elif ext == '.rar':
        parsed = parse_rar(path, width)
    elif ext == '.unitypackage':
        parsed = parse_unity(path, width)

    if not parsed:
        label_value("Type", try_file(path))
        print(f"\n  {D}(install python-rarfile/unrar for RAR, check file integrity){R}")

    print()


if __name__ == "__main__":
    main()
