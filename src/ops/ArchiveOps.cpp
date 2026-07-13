#include "ops/ArchiveOps.hpp"
#include "preview/ArchivePreview.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

static std::string ToLower(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

static std::string EscapePath(const std::string& path) {
    std::string escaped = path;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return "\"" + escaped + "\"";
}

static bool IsToolAvailable(const std::string& tool) {
    std::string cmd = "which " + tool + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buf[64];
    bool found = fgets(buf, sizeof(buf), pipe) != nullptr;
    pclose(pipe);
    return found;
}

static bool RunExtractCommand(const std::string& cmd,
                               std::function<void(const std::string&)> on_status,
                               std::function<bool()> is_cancelled) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;

    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        if (is_cancelled && is_cancelled()) {
            pclose(pipe);
            return false;
        }
        if (on_status) {
            std::string line(buf);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();
            if (!line.empty()) on_status(line);
        }
    }

    int rc = pclose(pipe);
    return rc == 0;
}

ArchiveFormat DetectArchiveFormat(const std::string& filePath) {
    auto name_lower = ToLower(fs::path(filePath).filename().string());
    auto ext = fs::path(name_lower).extension().string();

    if (ext == ".zip") return ArchiveFormat::Zip;

    if (name_lower.find(".tar.") != std::string::npos) {
        if (name_lower.find(".tar.gz") != std::string::npos ||
            name_lower.find(".tar.bz2") != std::string::npos) {
            auto inner = name_lower.substr(name_lower.find(".tar.") + 5);
            if (inner == "gz") return ArchiveFormat::Gzip;
            if (inner == "bz2") return ArchiveFormat::Bzip2;
            if (inner == "xz") return ArchiveFormat::Xz;
            if (inner == "zst") return ArchiveFormat::Zstd;
        }
        return ArchiveFormat::Tar;
    }

    if (ext == ".tgz") return ArchiveFormat::Gzip;
    if (ext == ".tbz2") return ArchiveFormat::Bzip2;
    if (ext == ".txz") return ArchiveFormat::Xz;
    if (ext == ".tzst") return ArchiveFormat::Zstd;
    if (ext == ".tar") return ArchiveFormat::Tar;
    if (ext == ".7z") return ArchiveFormat::SevenZ;
    if (ext == ".rar") return ArchiveFormat::Rar;
    if (ext == ".cab") return ArchiveFormat::Cab;
    if (ext == ".iso") return ArchiveFormat::Iso;

    return ArchiveFormat::Unknown;
}

std::string FormatName(ArchiveFormat fmt) {
    switch (fmt) {
    case ArchiveFormat::Zip:    return "ZIP";
    case ArchiveFormat::Tar:    return "TAR";
    case ArchiveFormat::Gzip:   return "GZip";
    case ArchiveFormat::Bzip2:  return "BZip2";
    case ArchiveFormat::Xz:     return "XZ";
    case ArchiveFormat::Zstd:   return "Zstd";
    case ArchiveFormat::SevenZ: return "7z";
    case ArchiveFormat::Rar:    return "RAR";
    case ArchiveFormat::Cab:    return "CAB";
    case ArchiveFormat::Iso:    return "ISO";
    default:                    return "Unknown";
    }
}

bool IsExtractToolAvailable(ArchiveFormat fmt) {
    switch (fmt) {
    case ArchiveFormat::Zip:    return IsToolAvailable("unzip");
    case ArchiveFormat::Tar:
    case ArchiveFormat::Gzip:
    case ArchiveFormat::Bzip2:
    case ArchiveFormat::Xz:
    case ArchiveFormat::Zstd:   return IsToolAvailable("tar");
    case ArchiveFormat::SevenZ:
    case ArchiveFormat::Cab:
    case ArchiveFormat::Iso:    return IsToolAvailable("7z");
    case ArchiveFormat::Rar:    return IsToolAvailable("unrar") || IsToolAvailable("7z");
    default:                    return false;
    }
}

bool ExtractArchive(const std::string& filePath, const std::string& destDir,
                    std::function<void(const std::string&)> on_status,
                    std::function<bool()> is_cancelled) {
    if (!fs::exists(filePath)) return false;

    auto fmt = DetectArchiveFormat(filePath);
    if (fmt == ArchiveFormat::Unknown) return false;

    if (!IsExtractToolAvailable(fmt)) return false;

    std::string cmd;
    switch (fmt) {
    case ArchiveFormat::Zip:
        cmd = "unzip -o " + EscapePath(filePath) + " -d " + EscapePath(destDir) + " 2>&1";
        break;
    case ArchiveFormat::Tar:
    case ArchiveFormat::Gzip:
    case ArchiveFormat::Bzip2:
    case ArchiveFormat::Xz:
    case ArchiveFormat::Zstd:
        cmd = "tar -xf " + EscapePath(filePath) + " -C " + EscapePath(destDir) + " 2>&1";
        break;
    case ArchiveFormat::SevenZ:
    case ArchiveFormat::Cab:
    case ArchiveFormat::Iso:
        cmd = "7z x " + EscapePath(filePath) + " -o" + EscapePath(destDir) + " -y 2>&1";
        break;
    case ArchiveFormat::Rar:
        if (IsToolAvailable("unrar"))
            cmd = "unrar x " + EscapePath(filePath) + " " + EscapePath(destDir) + "/ 2>&1";
        else
            cmd = "7z x " + EscapePath(filePath) + " -o" + EscapePath(destDir) + " -y 2>&1";
        break;
    default:
        return false;
    }

    if (on_status)
        on_status("Extracting: " + fs::path(filePath).filename().string());

    return RunExtractCommand(cmd, std::move(on_status), std::move(is_cancelled));
}
