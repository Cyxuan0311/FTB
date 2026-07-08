// FTB Plugin: os-icon
// Displays current OS platform with Nerd Font icon in the status bar
// Reads ftb.ctx.platform and ftb.ctx.distro_id from PluginContext
// (no subprocess, no file I/O — zero performance cost)

const DISTRO_ICONS: Record<string, string> = {
    "alpine":     "\u{F300}",
    "aosc":       "\u{F301}",
    "arch":       "\u{F303}",
    "archarm":    "\u{F303}",
    "artix":      "\u{F303}",
    "centos":     "\u{F304}",
    "coreos":     "\u{F305}",
    "debian":     "\u{F306}",
    "devuan":     "\u{F307}",
    "elementary": "\u{F309}",
    "fedora":     "\u{F30A}",
    "gentoo":     "\u{F30D}",
    "kali":       "\u{F306}",
    "linuxmint":  "\u{F30E}",
    "mageia":     "\u{F310}",
    "mandriva":   "\u{F311}",
    "manjaro":    "\u{F312}",
    "mint":       "\u{F30E}",
    "nixos":      "\u{F31A}",
    "opensuse":   "\u{F314}",
    "pop":        "\u{F31B}",
    "raspbian":   "\u{F315}",
    "rhel":       "\u{F316}",
    "rocky":      "\u{F316}",
    "sabayon":    "\u{F317}",
    "slackware":  "\u{F318}",
    "suse":       "\u{F314}",
    "ubuntu":     "\u{F31B}",
    "void":       "\u{F31D}",
    "zorin":      "\u{F320}",
};

const PLATFORM_ICONS: Record<string, { icon: string; label: string; fg: string; bg: string }> = {
    "linux":   { icon: "\u{F17C}", label: "Linux",   fg: "#ffffff", bg: "#e95420" },
    "macos":   { icon: "\u{F179}", label: "macOS",   fg: "#ffffff", bg: "#555555" },
    "windows": { icon: "\u{F17A}", label: "Windows", fg: "#ffffff", bg: "#00a4ef" },
    "wsl":     { icon: "\u{F17A}", label: "WSL",     fg: "#ffffff", bg: "#4d4d4d" },
    "freebsd": { icon: "\u{F30C}", label: "FreeBSD", fg: "#ffffff", bg: "#ab2b28" },
    "openbsd": { icon: "\u{F30C}", label: "OpenBSD", fg: "#ffffff", bg: "#000000" },
    "netbsd":  { icon: "\u{F30C}", label: "NetBSD",  fg: "#ffffff", bg: "#3d5a80" },
};

function entry(ftb: any): any[] {
    const platform: string = ftb.ctx.platform || "";
    const distroId: string = ftb.ctx.distro_id || "";

    const segments: any[] = [];
    let icon: string;
    let label: string;
    let fg: string;
    let bg: string;

    if (platform === "linux" && distroId && DISTRO_ICONS[distroId]) {
        icon = DISTRO_ICONS[distroId];
        label = distroId.charAt(0).toUpperCase() + distroId.slice(1);
        fg = "#ffffff";
        bg = "#1793d1";
    } else if (PLATFORM_ICONS[platform]) {
        const p = PLATFORM_ICONS[platform];
        icon = p.icon;
        label = p.label;
        fg = p.fg;
        bg = p.bg;
    } else {
        icon = "\u{F17C}";
        label = platform || "Unknown";
        fg = "#cdd6f4";
        bg = "#313244";
    }

    segments.push({
        text: " " + icon + " " + label + " ",
        fg: fg,
        bg: bg,
        bold: true,
        align: "left"
    });

    return segments;
}
