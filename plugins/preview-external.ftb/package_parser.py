#!/usr/bin/env python3
"""Software package analysis (deb, rpm, AppImage, snap, pkg.tar.*)."""

import sys
import os
import struct
import gzip
import tarfile
import subprocess
import shutil

from _common import *


# ─── AR archive reader (for .deb) ──────────────────────────────────

AR_MAGIC = b'!<arch>\n'
AR_HEADER_FMT = '<16s12s6s6s8s10s2s'
AR_HEADER_SIZE = 60

def read_ar_header(f):
    raw = f.read(AR_HEADER_SIZE)
    if len(raw) < AR_HEADER_SIZE:
        return None
    fields = struct.unpack(AR_HEADER_FMT, raw)
    name = fields[0].decode('ascii', errors='replace').rstrip()
    size = int(fields[5].decode('ascii', errors='replace').rstrip())
    return name, size

def read_ar_entry(f, size):
    data = f.read(size)
    if size % 2 == 1:
        f.read(1)
    return data


# ─── .deb ──────────────────────────────────────────────────────────

DEBIAN_FIELDS = ['Package', 'Version', 'Architecture', 'Maintainer',
                 'Description', 'Depends', 'Recommends', 'Suggests',
                 'Section', 'Priority', 'Homepage', 'Installed-Size',
                 'Essential', 'Pre-Depends', 'Breaks', 'Conflicts',
                 'Replaces', 'Provides']

def parse_deb(path, width):
    try:
        f = open(path, 'rb')
    except Exception:
        return False

    magic = f.read(len(AR_MAGIC))
    if magic != AR_MAGIC:
        f.close()
        return False

    section("Debian Package")

    control_data = None
    data_size = 0

    while True:
        hdr = read_ar_header(f)
        if hdr is None:
            break
        name, size = hdr
        if name == 'debian-binary':
            content = read_ar_entry(f, size)
            label_value("Format", content.decode('ascii', errors='replace').strip())
        elif name.startswith('control.tar'):
            raw = read_ar_entry(f, size)
            try:
                if name.endswith('.gz'):
                    tf = tarfile.open(fileobj=gzip.GzipFile(fileobj=__import__('io').BytesIO(raw)), mode='r')
                else:
                    tf = tarfile.open(fileobj=__import__('io').BytesIO(raw), mode='r')
                for m in tf:
                    if m.isfile() and (m.name == 'control' or m.name.endswith('/control')):
                        cf = tf.extractfile(m)
                        if cf:
                            control_data = cf.read().decode('utf-8', errors='replace')
                tf.close()
            except Exception:
                pass
        elif name.startswith('data.tar'):
            data_size = size
            read_ar_entry(f, size)
            try:
                ar_f = open(path, 'rb')
                ar_f.read(len(AR_MAGIC))
                found_data = False
                file_count = 0
                while True:
                    h = read_ar_header(ar_f)
                    if h is None:
                        break
                    n, s = h
                    if n.startswith('data.tar'):
                        found_data = True
                        if n.endswith('.gz'):
                            tf = tarfile.open(fileobj=gzip.GzipFile(fileobj=ar_f), mode='r|')
                        elif n.endswith('.xz'):
                            tf = tarfile.open(fileobj=ar_f, mode='r|xz')
                        else:
                            tf = tarfile.open(fileobj=ar_f, mode='r|')
                        for m in tf:
                            if m.isfile():
                                file_count += 1
                        tf.close()
                        break
                    else:
                        read_ar_entry(ar_f, s)
                ar_f.close()
                label_value("Files", str(file_count))
            except Exception:
                pass
        else:
            read_ar_entry(f, size)

    f.close()

    if control_data:
        section("Control Fields")
        for line in control_data.split('\n'):
            for field in DEBIAN_FIELDS:
                if line.startswith(field + ':'):
                    val = line[len(field) + 1:].strip()
                    label_value(field, val)
                    break

    return True


# ─── .rpm ──────────────────────────────────────────────────────────

def parse_rpm(path, width):
    if not shutil.which('rpm'):
        return False

    try:
        r = subprocess.run(
            ['rpm', '-qip', '--queryformat',
             'Name: %{NAME}\nVersion: %{VERSION}\nRelease: %{RELEASE}\n'
             'Architecture: %{ARCH}\nGroup: %{GROUP}\n'
             'License: %{LICENSE}\nVendor: %{VENDOR}\n'
             'Packager: %{PACKAGER}\nURL: %{URL}\nBuild Host: %{BUILDHOST}\n'
             'Install Size: %{SIZE}\nSummary: %{SUMMARY}\nDescription: %{DESCRIPTION}\n',
             path],
            capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            return False
    except Exception:
        return False

    section("RPM Package")
    summary = ""
    for line in r.stdout.split('\n'):
        if ':' in line:
            key, val = line.split(':', 1)
            key = key.strip()
            val = val.strip()
            if val:
                if key == 'Summary':
                    summary = val
                elif key == 'Description':
                    label_value(key, val[:300])
                elif key == 'Install Size':
                    try:
                        label_value(key, fmt_size(int(val) * 1024))
                    except Exception:
                        label_value(key, val)
                else:
                    label_value(key, val)

    if summary:
        label_value("Summary", summary)

    return True


# ─── AppImage ──────────────────────────────────────────────────────

def parse_appimage(path, width):
    with open(path, 'rb') as f:
        magic = f.read(4)

    is_elf = magic[:4] == b'\x7fELF'

    section("AppImage")

    if is_elf:
        label_value("Format", "ELF 64-bit executable (AppImage)")

    try:
        r = subprocess.run(['file', '-Lb', path], capture_output=True, text=True, timeout=5)
        if r.returncode == 0 and r.stdout.strip():
            label_value("Type", r.stdout.strip())
    except Exception:
        pass

    try:
        r = subprocess.run([path, '--appimage-info'], capture_output=True, text=True, timeout=5)
        if r.returncode == 0 and r.stdout.strip():
            label_value("AppImage Info", r.stdout.strip()[:500])
    except Exception:
        pass

    return True


# ─── Snap ──────────────────────────────────────────────────────────

def parse_snap(path, width):
    section("Snap Package")
    try:
        r = subprocess.run(['file', '-Lb', path], capture_output=True, text=True, timeout=5)
        if r.returncode == 0 and r.stdout.strip():
            label_value("Type", r.stdout.strip())
    except Exception:
        pass

    try:
        r = subprocess.run(['unsquashfs', '-ll', path],
                          capture_output=True, text=True, timeout=10)
        if r.returncode == 0:
            lines = r.stdout.strip().split('\n')
            file_count = sum(1 for l in lines if not l.startswith('d'))
            label_value("Files", str(file_count))
    except Exception:
        pass

    return True


# ─── .pkg.tar.* (Arch Linux) ──────────────────────────────────────

def parse_pkg_tar(path, width):
    ext = os.path.splitext(path)[1].lower()
    secondary = os.path.splitext(os.path.splitext(path)[0])[1].lower() if path.count('.') > 1 else ''

    mode = None
    if ext == '.xz' or secondary == '.xz':
        mode = 'r|xz'
    elif ext == '.zst' or secondary == '.zst':
        pass
    elif ext == '.gz' or secondary == '.gz':
        mode = 'r|gz'

    if mode is None:
        return False

    section("Arch Package (tar{})".format({'r|xz': '.xz', 'r|gz': '.gz'}.get(mode, '')))

    try:
        tar = tarfile.open(path, mode)
    except Exception:
        return False

    file_count = 0
    total_size = 0
    etc_files = 0
    bin_files = 0
    lib_files = 0
    first_entries = []
    pkg_desc = None

    try:
        for member in tar:
            if member.isfile():
                file_count += 1
                total_size += member.size
                name = member.name
                if name.startswith('etc/'):
                    etc_files += 1
                elif '/bin/' in name or name.startswith('usr/bin/'):
                    bin_files += 1
                elif name.startswith('usr/lib/') or name.startswith('lib/'):
                    lib_files += 1

                if len(first_entries) < 20:
                    first_entries.append(name)

                if name == '.PKGINFO' and pkg_desc is None:
                    cf = tar.extractfile(member)
                    if cf:
                        pkg_desc = cf.read().decode('utf-8', errors='replace')

            if file_count >= 2000 and pkg_desc is not None:
                break
    except Exception:
        pass
    finally:
        tar.close()

    if pkg_desc:
        section("PKGINFO")
        for line in pkg_desc.split('\n'):
            if '=' in line:
                k, v = line.split('=', 1)
                label_value(k.strip(), v.strip())

    section("Contents")
    label_value("Files", str(file_count))
    label_value("Uncompressed Size", fmt_size(total_size))
    if etc_files:
        label_value("Config files", str(etc_files))
    if bin_files:
        label_value("Binaries", str(bin_files))
    if lib_files:
        label_value("Libraries", str(lib_files))

    if first_entries:
        print(f"  {D}First entries:{R}")
        for entry in first_entries[:15]:
            print(f"    {W}{entry}{R}")
        if len(first_entries) > 15:
            print(f"    {D}... and {len(first_entries) - 15} more{R}")

    return True


# ─── Main ──────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: package_parser.py <package_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()
    basename = os.path.basename(path)

    print(f"\n{B}[ Package Analysis ]{R}")
    label_value("File", basename)
    label_value("Size", fmt_size(os.path.getsize(path)))

    parsed = False

    if ext == '.deb':
        parsed = parse_deb(path, width)
    elif ext == '.rpm':
        parsed = parse_rpm(path, width)
    elif ext == '.AppImage' or ext == '.appimage':
        parsed = parse_appimage(path, width)
    elif ext == '.snap':
        parsed = parse_snap(path, width)
    elif ext in ('.zst', '.xz', '.gz') and 'pkg' in basename:
        parsed = parse_pkg_tar(path, width)

    if not parsed:
        try:
            r = subprocess.run(['file', '-Lb', path], capture_output=True, text=True, timeout=5)
            label_value("Type", r.stdout.strip() if r.returncode == 0 else "unknown")
        except Exception:
            label_value("Type", "unknown")
        print(f"\n  {D}(install rpm for .rpm, or check file integrity){R}")

    print()


if __name__ == "__main__":
    main()
