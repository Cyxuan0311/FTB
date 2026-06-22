#include "FTB/ArchivePreview.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <memory>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

static std::string ToLower(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// ---- 安全的 shell 命令执行 (输出限制 64KB) ----
static std::string RunCommand(const std::string& cmd) {
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return result;
    char buf[4096];
    size_t total = 0;
    while (total < 65536 && fgets(buf, sizeof(buf), pipe.get())) {
        size_t len = strlen(buf);
        if (total + len > 65536) {
            len = 65536 - total;
            result.append(buf, len);
            break;
        }
        result.append(buf, len);
        total += len;
    }
    return result;
}

// ---- 对文件路径做 shell 转义 (用双引号包裹) ----
static std::string EscapePath(const std::string& path) {
    std::string escaped = path;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return "\"" + escaped + "\"";
}

// ---- 检测已知档案扩展名 ----
static bool HasArchiveExt(const std::string& name_lower) {
    if (name_lower.find(".tar.") != std::string::npos) return true;
    auto ext = fs::path(name_lower).extension().string();
    return ext == ".zip" || ext == ".tgz" || ext == ".tbz2" ||
           ext == ".txz" || ext == ".tzst" || ext == ".7z" ||
           ext == ".rar" || ext == ".cab" || ext == ".iso";
}

// ============================================================
// 解析器: unzip -l
// ============================================================
static std::vector<ArchiveEntry> ParseUnzipOutput(const std::string& output) {
    std::vector<ArchiveEntry> entries;
    std::istringstream iss(output);
    std::string line;
    int line_num = 0;
    while (std::getline(iss, line)) {
        line_num++;
        if (line_num <= 3) continue;
        if (line.find("---------") != std::string::npos) break;
        std::regex re(R"(^\s*(\d+)\s+\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}\s+(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, re)) {
            ArchiveEntry e;
            e.size = std::stoull(m[1]);
            e.name = m[2];
            e.is_dir = (!e.name.empty() && e.name.back() == '/');
            entries.push_back(std::move(e));
        }
    }
    return entries;
}

// ============================================================
// 解析器: tar -tvf
// ============================================================
static std::vector<ArchiveEntry> ParseTarOutput(const std::string& output) {
    std::vector<ArchiveEntry> entries;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        // 格式: permissions owner size date time name
        // GNU tar: -rw-r--r-- user/group 1234 2024-01-01 12:00 path
        // BSD tar: 类似但有差异; 兼容处理
        std::regex re(R"(^[drwxst-]{10}\s+\S+\s+(\d+)\s+\S+\s+\S+\s+(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, re)) {
            ArchiveEntry e;
            e.size = std::stoull(m[1]);
            e.name = m[2];
            e.is_dir = (!e.name.empty() && e.name.back() == '/');
            entries.push_back(std::move(e));
        }
    }
    return entries;
}

// ============================================================
// 解析器: 7z l
// ============================================================
static std::vector<ArchiveEntry> Parse7zOutput(const std::string& output) {
    std::vector<ArchiveEntry> entries;
    std::istringstream iss(output);
    std::string line;
    bool in_table = false;
    while (std::getline(iss, line)) {
        // 找到分隔线进入表格区域
        if (line.find("-------------------") != std::string::npos) {
            if (!in_table) {
                in_table = true;
                continue; // 跳过标题行和第一个分隔线
            } else {
                break; // 第二个分隔线，表格结束
            }
        }
        if (!in_table) continue;

        // 跳过空行
        if (line.empty()) continue;

        // 解析: Date Time Attr Size Compressed Name
        // 2024-01-01 12:00:00 D.... 0 0 path/to/dir
        // 2024-01-01 12:00:00 ....A 1234 567 path/to/file
        std::regex re(R"(^\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\s+([\.D]{5})\s+(\d+)\s+\d+\s+(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, re)) {
            ArchiveEntry e;
            e.is_dir = (m[1].str()[0] == 'D');
            e.size = e.is_dir ? 0 : std::stoull(m[2]);
            e.name = m[3];
            entries.push_back(std::move(e));
        }
    }
    return entries;
}

// ============================================================
// 解析器: isoinfo -l (如果可用)
// ============================================================
static std::vector<ArchiveEntry> ParseIsoinfoOutput(const std::string& output) {
    std::vector<ArchiveEntry> entries;
    std::istringstream iss(output);
    std::string line;
    // isoinfo -l 输出格式:
    // Directory listing of /
    // d---------   0  2024-01-01 12:00   dir/
    // ----------  1234  2024-01-01 12:00   file.txt
    while (std::getline(iss, line)) {
        if (line.find("Directory listing of") != std::string::npos) continue;
        std::regex re(R"(^[d-]---------[\s]+(\d+)[\s]+\S+[\s]+\S+[\s]+(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, re)) {
            ArchiveEntry e;
            e.size = std::stoull(m[1]);
            e.name = m[2];
            e.is_dir = (!e.name.empty() && e.name.back() == '/');
            entries.push_back(std::move(e));
        }
    }
    return entries;
}

// ---- 根据扩展名判断使用哪个命令 ----
static std::vector<ArchiveEntry> ListWithCommands(const std::string& filePath,
                                                   const std::string& escaped) {
    auto name_lower = ToLower(fs::path(filePath).filename().string());
    auto ext = fs::path(name_lower).extension().string();

    // Tier 1: 直接匹配已知格式
    if (ext == ".zip") {
        auto out = RunCommand("unzip -l " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = ParseUnzipOutput(out);
            if (!entries.empty()) return entries;
        }
    }

    if (ext == ".tar" || name_lower.find(".tar.") != std::string::npos ||
        ext == ".tgz" || ext == ".tbz2" || ext == ".txz" || ext == ".tzst") {
        auto out = RunCommand("tar -tvf " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = ParseTarOutput(out);
            if (!entries.empty()) return entries;
        }
    }

    if (ext == ".7z" || ext == ".rar" || ext == ".cab") {
        auto out = RunCommand("7z l " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = Parse7zOutput(out);
            if (!entries.empty()) return entries;
        }
    }

    if (ext == ".iso") {
        auto out = RunCommand("isoinfo -l " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = ParseIsoinfoOutput(out);
            if (!entries.empty()) return entries;
        }
    }

    // Tier 2: 兜底 — 依次尝试 7z, tar, unzip
    {
        auto out = RunCommand("7z l " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = Parse7zOutput(out);
            if (!entries.empty()) return entries;
        }
    }
    {
        auto out = RunCommand("tar -tvf " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = ParseTarOutput(out);
            if (!entries.empty()) return entries;
        }
    }
    {
        auto out = RunCommand("unzip -l " + escaped + " 2>/dev/null | head -n 60");
        if (!out.empty()) {
            auto entries = ParseUnzipOutput(out);
            if (!entries.empty()) return entries;
        }
    }

    return {};
}

// ============================================================
// 公开 API
// ============================================================
bool IsArchiveFile(const std::string& name) {
    std::string name_lower = ToLower(name);
    if (name_lower.find(".tar.") != std::string::npos) return true;
    auto ext = fs::path(name_lower).extension().string();
    return ext == ".zip" || ext == ".tgz" || ext == ".tbz2" ||
           ext == ".txz" || ext == ".tzst" || ext == ".7z" ||
           ext == ".rar" || ext == ".cab" || ext == ".iso";
}

std::vector<ArchiveEntry> ListArchiveContents(const std::string& filePath) {
    auto escaped = EscapePath(filePath);
    return ListWithCommands(filePath, escaped);
}
