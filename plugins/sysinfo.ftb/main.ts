const CPU_STATE_FILE = "/tmp/ftb_sysinfo_cpu.json";
const NET_CACHE_FILE = "/tmp/ftb_sysinfo_net.json";
const NET_CACHE_TTL = 10000; // ms
const NET_SPEED_FILE = "/tmp/ftb_sysinfo_net_speed.json";

function getCpuUsage(ftb) {
    let statContent;
    try {
        statContent = ftb.fs.readFile("/proc/stat");
        if (!statContent) return null;
    } catch {
        return null;
    }

    const firstLine = statContent.split("\n")[0];
    if (!firstLine || firstLine.indexOf("cpu ") !== 0) return null;

    const parts = firstLine.trim().split(/\s+/).slice(1);
    if (parts.length < 5) return null;

    const user    = parseInt(parts[0]) || 0;
    const nice    = parseInt(parts[1]) || 0;
    const system  = parseInt(parts[2]) || 0;
    const idle    = parseInt(parts[3]) || 0;
    const iowait  = parseInt(parts[4]) || 0;
    const irq     = parseInt(parts[5]) || 0;
    const softirq = parseInt(parts[6]) || 0;
    const steal   = parseInt(parts[7]) || 0;

    const total = user + nice + system + idle + iowait + irq + softirq + steal;

    let prev = null;
    try {
        const raw = ftb.fs.readFile(CPU_STATE_FILE);
        if (raw) prev = JSON.parse(raw);
    } catch {}

    ftb.fs.writeFile(CPU_STATE_FILE, JSON.stringify({ total, idle }));

    if (!prev) return null;

    const deltaTotal = total - prev.total;
    const deltaIdle  = idle  - prev.idle;

    if (deltaTotal <= 0) return 0;

    return Math.max(0, Math.min(100, Math.round((1 - deltaIdle / deltaTotal) * 100)));
}

function findBatteryPath(ftb) {
    let entries;
    try {
        entries = ftb.fs.listDir("/sys/class/power_supply");
    } catch {
        return null;
    }
    if (!entries || entries.length === 0) return null;

    for (let i = 0; i < entries.length; i++) {
        const name = entries[i];
        try {
            const type = ftb.fs.readFile("/sys/class/power_supply/" + name + "/type");
            if (type && type.trim() === "Battery") {
                return "/sys/class/power_supply/" + name;
            }
        } catch {}
    }
    return null;
}

function getBattery(ftb) {
    const batPath = findBatteryPath(ftb);
    if (!batPath) return null;

    try {
        const capStr = ftb.fs.readFile(batPath + "/capacity");
        if (!capStr) return null;
        const percent = parseInt(capStr.trim());
        if (isNaN(percent)) return null;

        let charging = false;
        try {
            const status = ftb.fs.readFile(batPath + "/status");
            charging = status && status.trim() === "Charging";
        } catch {}

        return { percent, charging };
    } catch {
        return null;
    }
}

function readNetCache(ftb) {
    try {
        const raw = ftb.fs.readFile(NET_CACHE_FILE);
        if (raw) {
            const data = JSON.parse(raw);
            if (data && data.ssid && (Date.now() - data.ts) < NET_CACHE_TTL) {
                return { ssid: data.ssid, status: "wifi" };
            }
        }
    } catch {}
    return null;
}

function writeNetCache(ftb, ssid) {
    try {
        var now = Date.now();
        ftb.fs.writeFile(NET_CACHE_FILE, JSON.stringify({ ssid: ssid, ts: now }));
    } catch {}
}

function detectNetwork(ftb) {
    // 1) Try iwgetid (native Linux)
    try {
        const found = ftb.exec("command", ["-v", "iwgetid"]);
        if (found && found.trim().length > 0) {
            const out = ftb.exec("iwgetid", ["-r"]);
            if (out && out.trim().length > 0) {
                return { ssid: out.trim(), status: "wifi" };
            }
        }
    } catch {}

    // 2) WSL: cmd.exe /c netsh wlan show interfaces
    var cmdPaths = ["cmd.exe", "/mnt/c/Windows/System32/cmd.exe"];
    for (var ci = 0; ci < cmdPaths.length; ci++) {
        try {
            var out = ftb.exec(cmdPaths[ci], ["/c", "netsh wlan show interfaces"]);
            if (out && out.trim().length > 0 && out.indexOf("/bin/sh:") === -1) {
                var lines = out.split("\n");
                for (var li = 0; li < lines.length; li++) {
                    var line = lines[li].trim();
                    if (line.indexOf("SSID") !== -1 || line.indexOf("ssid") !== -1) {
                        var parts = line.split(":");
                        if (parts.length >= 2) {
                            var ssid = parts.slice(1).join(":").trim();
                            if (ssid.length > 0) {
                                return { ssid: ssid, status: "wifi" };
                            }
                        }
                    }
                }
            }
        } catch {}
    }

    // 3) Check /proc/net/route for default gateway
    try {
        const content = ftb.fs.readFile("/proc/net/route");
        if (content) {
            const lines = content.split("\n");
            for (let i = 1; i < lines.length; i++) {
                const parts = lines[i].trim().split(/\s+/);
                if (parts.length >= 2 && parts[1] === "00000000") {
                    return { status: "wired" };
                }
            }
        }
    } catch {}

    // 4) Check /proc/net/wireless
    try {
        const wireless = ftb.fs.readFile("/proc/net/wireless");
        if (wireless) {
            const lines = wireless.split("\n");
            if (lines.length > 2 && lines[2].trim().length > 0) {
                return { status: "disconnected" };
            }
        }
    } catch {}

    return { status: "disconnected" };
}

function getNetwork(ftb) {
    // Try cache first (avoids slow WSL→Windows calls every cycle)
    var cached = readNetCache(ftb);
    if (cached) return cached;

    var result = detectNetwork(ftb);

    // Cache WiFi result for next cycles
    if (result.ssid) {
        writeNetCache(ftb, result.ssid);
    }

    return result;
}

function getNetworkSpeed(ftb) {
    try {
        var content = ftb.fs.readFile("/proc/net/dev");
        if (!content) return null;

        var lines = content.split("\n");
        var iface = null;
        var rxBytes = 0, txBytes = 0;

        for (var i = 2; i < lines.length; i++) {
            var line = lines[i].trim();
            if (line.length === 0) continue;
            var nameEnd = line.indexOf(":");
            if (nameEnd === -1) continue;
            var name = line.substring(0, nameEnd).trim();
            if (name === "lo") continue;
            iface = name;
            var rest = line.substring(nameEnd + 1).trim().split(/\s+/);
            if (rest.length >= 10) {
                rxBytes = parseInt(rest[0]) || 0;
                txBytes = parseInt(rest[8]) || 0;
            }
            break;
        }

        if (!iface) return null;

        var now = Date.now();

        // Read previous sample
        var prevRaw;
        try { prevRaw = ftb.fs.readFile(NET_SPEED_FILE); } catch {}

        // Write current sample
        ftb.fs.writeFile(NET_SPEED_FILE, JSON.stringify({ rx: rxBytes, tx: txBytes, ts: now }));

        if (!prevRaw) return null;

        var prev = JSON.parse(prevRaw);
        if (!prev || prev.rx === undefined) return null;

        var dt = (now - prev.ts) / 1000; // seconds
        if (dt <= 0) return null;

        var drx = rxBytes - prev.rx;
        var dtx = txBytes - prev.tx;

        if (drx < 0) drx = 0;
        if (dtx < 0) dtx = 0;

        return {
            rxSpeed: drx / dt,
            txSpeed: dtx / dt
        };
    } catch {}
    return null;
}

function formatSpeed(bytesPerSec) {
    if (bytesPerSec >= 1000000) {
        return (bytesPerSec / 1000000).toFixed(1) + "M";
    }
    if (bytesPerSec >= 100) {
        return (bytesPerSec / 1000).toFixed(1) + "K";
    }
    return "0";
}

function entry(ftb) {
    const segments = [];

    // Direct /proc reads
    const cpu = getCpuUsage(ftb);
    if (cpu !== null) {
        segments.push({
            text: " CPU " + cpu + "% ",
            fg: "#cdd6f4",
            bg: "#313244",
            bold: false,
            align: "right"
        });
    }

    const net = getNetwork(ftb);
    if (net) {
        var netText, netColor;
        if (net.ssid) {
            netText = " WiFi " + net.ssid + " ";
            netColor = "#89b4fa";
        } else if (net.status === "wired") {
            netText = " Eth ";
            netColor = "#a6e3a1";
        } else {
            netText = " No Net ";
            netColor = "#f38ba8";
        }

        // Append network speed
        var speed = getNetworkSpeed(ftb);
        if (speed) {
            netText = netText.substring(0, netText.length - 1) + " \u2193" + formatSpeed(speed.rxSpeed) + " \u2191" + formatSpeed(speed.txSpeed) + " ";
        }

        segments.push({
            text: netText,
            fg: netColor,
            bg: "#313244",
            bold: false,
            align: "right"
        });
    }

    const bat = getBattery(ftb);
    if (bat !== null) {
        const label = bat.charging ? " Bat " + bat.percent + "% + " : " Bat " + bat.percent + "% ";
        segments.push({
            text: label,
            fg: bat.charging ? "#f9e2af" : "#a6e3a1",
            bg: "#313244",
            bold: false,
            align: "right"
        });
    }

    return segments;
}
