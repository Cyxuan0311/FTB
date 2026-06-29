const CPU_STATE_FILE = "/tmp/ftb_sysinfo_cpu.json";

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

function entry(ftb) {
    const segments = [];

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
