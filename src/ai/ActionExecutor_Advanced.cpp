#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "utils/PerfLogger.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleCreateSymlink(const ToolCall& call) {
    ExecutionResult result;
    auto target = resolvePath(call.params["target"].get<std::string>(), state_.currentPath);
    auto link_path = resolvePath(call.params["link_path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(target) || !isPathSafe(link_path)) { result.message = "Access denied"; return result; }

    bool relative = call.params.value("relative", false);

    std::error_code ec;
    if (relative) {
        auto rel = fs::relative(target, fs::path(link_path).parent_path(), ec);
        if (ec) { result.message = "Failed to compute relative path"; return result; }
        fs::create_symlink(rel, link_path, ec);
    } else {
        fs::create_symlink(target, link_path, ec);
    }
    if (ec) { result.message = "Failed to create symlink: " + ec.message(); return result; }
    result.success = true;
    result.message = "Created symlink " + fs::path(link_path).filename().string() + " -> " + target;
    return result;
}

ExecutionResult ActionExecutor::handleDiffFiles(const ToolCall& call) {
    ExecutionResult result;
    auto file1 = resolvePath(call.params["file1"].get<std::string>(), state_.currentPath);
    auto file2 = resolvePath(call.params["file2"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(file1) || !isPathSafe(file2)) { result.message = "Access denied"; return result; }

    std::ifstream f1(file1), f2(file2);
    if (!f1) { result.message = "Cannot open " + file1; return result; }
    if (!f2) { result.message = "Cannot open " + file2; return result; }

    std::vector<std::string> lines1, lines2;
    std::string line;
    while (std::getline(f1, line)) lines1.push_back(line);
    while (std::getline(f2, line)) lines2.push_back(line);

    std::stringstream ss;
    ss << "--- " << file1 << "\n+++ " << file2 << "\n";
    size_t i = 0, j = 0;
    int context = 3;
    while (i < lines1.size() || j < lines2.size()) {
        if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j]) {
            if (context > 0) { ss << " " << lines1[i] << "\n"; --context; }
            ++i; ++j;
        } else {
            context = 3;
            if (i < lines1.size()) {
                ss << "-" << lines1[i] << "\n";
                ++i;
            }
            if (j < lines2.size()) {
                ss << "+" << lines2[j] << "\n";
                ++j;
            }
        }
    }

    result.success = true;
    result.message = ss.str();
    return result;
}

static bool stringEndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

ExecutionResult ActionExecutor::handleCompress(const ToolCall& call) {
    ExecutionResult result;
    if (!call.params.contains("paths") || !call.params["paths"].is_array()) {
        result.message = "paths must be an array";
        return result;
    }

    auto paths = call.params["paths"].get<std::vector<std::string>>();
    auto archive_name = call.params["archive_name"].get<std::string>();
    std::string format = call.params.value("format", "zip");

    std::string cmd;
    if (format == "zip") {
        if (!stringEndsWith(archive_name, ".zip")) archive_name += ".zip";
        cmd = "cd " + state_.currentPath + " && zip -r " + archive_name;
        for (const auto& p : paths) cmd += " " + p;
    } else if (format == "tar.gz") {
        if (!stringEndsWith(archive_name, ".tar.gz")) archive_name += ".tar.gz";
        cmd = "cd " + state_.currentPath + " && tar -czf " + archive_name;
        for (const auto& p : paths) cmd += " " + p;
    } else {
        result.message = "Unsupported format: " + format;
        return result;
    }

    int rc = std::system(cmd.c_str());
    if (rc != 0) { result.message = "Compression failed with code " + std::to_string(rc); return result; }
    result.success = true;
    result.message = "Created archive " + archive_name;
    return result;
}

ExecutionResult ActionExecutor::handleExtract(const ToolCall& call) {
    ExecutionResult result;
    auto archive = resolvePath(call.params["archive"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(archive)) { result.message = "Access denied"; return result; }

    std::string dest;
    if (call.params.contains("destination")) {
        dest = resolvePath(call.params["destination"].get<std::string>(), state_.currentPath);
    } else {
        dest = state_.currentPath;
    }

    int rc;
    auto ext = fs::path(archive).extension().string();
    if (ext == ".zip") {
        rc = std::system(("cd " + dest + " && unzip -o " + archive).c_str());
    } else if (ext == ".tar" || ext == ".gz" || ext == ".bz2" || ext == ".xz") {
        rc = std::system(("cd " + dest + " && tar -xf " + archive).c_str());
    } else {
        result.message = "Unsupported archive format: " + ext;
        return result;
    }

    if (rc != 0) { result.message = "Extraction failed with code " + std::to_string(rc); return result; }
    result.success = true;
    result.message = "Extracted " + fs::path(archive).filename().string() + " to " + dest;
    return result;
}

} // namespace FTB
