#!/usr/bin/env python3
"""MP3/audio file metadata extraction."""

import sys
import os
import json

from _common import *


def fmt_duration(secs):
    if secs is None:
        return "N/A"
    secs = int(secs)
    h, m, s = secs // 3600, (secs % 3600) // 60, secs % 60
    if h > 0:
        return f"{h}h {m:02d}m {s:02d}s"
    return f"{m}m {s:02d}s"


def try_mutagen(path):
    try:
        from mutagen import File as MFile
        audio = MFile(path)
        if audio is None:
            return None
    except Exception:
        return None

    info = {}

    if hasattr(audio, 'tags') and audio.tags:
        tags = audio.tags
        for key in ('title', 'artist', 'album', 'date', 'genre', 'tracknumber', 'discnumber', 'albumartist'):
            try:
                val = tags.get(key)
                if val is not None:
                    info[key] = str(val[0]) if isinstance(val, list) else str(val)
            except Exception:
                pass

    if hasattr(audio.info, 'length'):
        info['duration'] = audio.info.length
    if hasattr(audio.info, 'bitrate'):
        info['bitrate'] = audio.info.bitrate
    if hasattr(audio.info, 'sample_rate'):
        info['sample_rate'] = audio.info.sample_rate
    if hasattr(audio.info, 'channels'):
        info['channels'] = audio.info.channels

    return info


def try_ffprobe(path):
    try:
        import subprocess
        cmd = [
            'ffprobe', '-v', 'quiet', '-print_format', 'json',
            '-show_format', '-show_streams', path
        ]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            return None
        data = json.loads(r.stdout)
        info = {}
        fmt = data.get('format', {})
        if fmt.get('duration'):
            info['duration'] = float(fmt['duration'])
        if fmt.get('bit_rate'):
            info['bitrate'] = int(fmt['bit_rate'])
        for s in data.get('streams', []):
            if s.get('codec_type') == 'audio':
                if s.get('sample_rate'):
                    info['sample_rate'] = int(s['sample_rate'])
                if s.get('channels'):
                    info['channels'] = s['channels']
                if s.get('codec_name'):
                    info['codec'] = s['codec_name']
                if 'tags' in s:
                    for key in ('title', 'artist', 'album', 'date', 'genre', 'track', 'disc'):
                        val = s['tags'].get(key)
                        if val:
                            info[key.lower()] = val
                break
        if 'tags' in fmt:
            for key in ('title', 'artist', 'album', 'date', 'genre', 'track', 'disc'):
                val = fmt['tags'].get(key)
                if val and key.lower() not in info:
                    info[key.lower()] = val
        return info
    except Exception:
        return None


def main():
    if len(sys.argv) < 2:
        print("Usage: mp3_parser.py <audio_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()
    tag = ext.lstrip('.').upper() if ext else "UNKNOWN"

    print(f"\n{B}[ {tag} Audio Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))

    info = try_mutagen(path)
    if info is None:
        info = try_ffprobe(path)

    if info is None:
        label_value("Type", try_file(path))
        print(f"\n  {W}(install mutagen or ffprobe for detailed audio info){R}")
        sys.exit(0)

    if info.get('title') or info.get('artist') or info.get('album'):
        section("Metadata")
        if info.get('title'):
            label_value("Title", info['title'])
        if info.get('artist'):
            label_value("Artist", info['artist'])
        if info.get('albumartist'):
            label_value("Album Artist", info['albumartist'])
        if info.get('album'):
            label_value("Album", info['album'])
        if info.get('date') or info.get('year'):
            label_value("Year", info.get('date') or info.get('year', 'N/A'))
        if info.get('genre'):
            label_value("Genre", info['genre'])
        tn = info.get('tracknumber') or info.get('track', '')
        if tn:
            label_value("Track", tn)

    section("Audio Info")
    label_value("Duration", fmt_duration(info.get('duration')))
    if info.get('bitrate'):
        br = info['bitrate']
        label_value("Bitrate", f"{br // 1000} kbps" if br >= 1000 else f"{br} bps")
    if info.get('sample_rate'):
        label_value("Sample Rate", f"{info['sample_rate']} Hz")
    if info.get('channels'):
        ch = info['channels']
        ch_str = {1: "Mono", 2: "Stereo", 6: "5.1", 8: "7.1"}.get(ch, f"{ch} channels")
        label_value("Channels", ch_str)
    if info.get('codec'):
        label_value("Codec", info['codec'])

    print()


if __name__ == "__main__":
    main()
