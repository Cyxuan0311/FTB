#include "FTB/FuzzyFinder.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

static std::string ToLower(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

static std::string ShellEscape(const std::string& s) {
    std::string escaped = s;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return "\"" + escaped + "\"";
}

// 读取命令全部输出 (无大小限制)
static std::string RunCommand(const std::string& cmd) {
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return result;
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe.get())) > 0)
        result.append(buf, n);
    return result;
}

// ---- 模糊匹配: 按序匹配字符 (不必须连续) ----
bool FuzzyMatch(const std::string& query, const std::string& str) {
    if (query.empty()) return true;
    size_t qi = 0;
    for (size_t si = 0; si < str.size() && qi < query.size(); si++) {
        // 大小写不敏感: 对每个字符做 tolower 避免拷贝
        char sc = static_cast<char>(std::tolower(static_cast<unsigned char>(str[si])));
        char qc = static_cast<char>(std::tolower(static_cast<unsigned char>(query[qi])));
        if (sc == qc) qi++;
    }
    return qi == query.size();
}

// ---- 常见大型缓存/构建目录 (遍历时跳过) ----
static const std::unordered_set<std::string> s_skip_dirs = {
    ".git", ".svn", ".hg",
    "node_modules", ".npm", ".yarn",
    "target", "build", "dist", ".cache",
    "__pycache__", ".pytest_cache",
    ".m2", ".idea", ".vscode",
    ".next", ".nuxt", ".turbo",
    "venv", ".venv", ".tox",
    "bazel-out", "bazel-bin", "bazel-testlogs",
};

// ---- 检测外部 fdfind/fd 命令是否存在 ----
bool SearchEngine::HasExternal() {
    static int s_has = -1;
    if (s_has == -1) {
        auto r1 = RunCommand("which fdfind 2>/dev/null");
        auto r2 = RunCommand("which fd 2>/dev/null");
        s_has = (!r1.empty() || !r2.empty()) ? 1 : 0;
    }
    return s_has == 1;
}

// ---- 外部搜索: 调用 fdfind / fd ----
std::vector<FdResult> SearchEngine::ExternalSearch(const std::string& query,
                                                    const std::string& basePath) {
    std::string escaped_query = ShellEscape(query);
    std::string escaped_path = ShellEscape(basePath);
    // 只搜索文件, 不搜索目录
    std::string cmd =
        "(fdfind -t f --color never --max-depth 8 " +
        escaped_query + " " + escaped_path +
        " 2>/dev/null || fd -t f --color never --max-depth 8 " +
        escaped_query + " " + escaped_path +
        " 2>/dev/null)";

    auto output = RunCommand(cmd);
    if (output.empty()) return {};

    // 预分配: 根据换行符数量估算
    size_t estimated = 0;
    for (char c : output) if (c == '\n') estimated++;
    std::vector<FdResult> results;
    results.reserve(estimated);

    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        results.push_back({std::move(line), false});
    }

    return results;
}

// ---- 内置搜索: C++ 遍历目录 (无结果上限) ----
std::vector<FdResult> SearchEngine::BuiltinSearch(const std::string& query,
                                                   const std::string& basePath) {
    std::vector<FdResult> results;
    if (query.empty()) return results;
    results.reserve(4096); // 预分配避免反复扩容

    auto q_lower = ToLower(query);
    fs::path base = fs::absolute(basePath);
    std::string base_str = base.string();
    if (!base_str.empty() && base_str.back() != '/') base_str += '/';
    size_t base_len = base_str.size();

    std::error_code ec;

    // 手动递归遍历 (比 recursive_directory_iterator 灵活, 可跳过目录)
    std::function<void(const fs::path&, int)> walk =
        [&](const fs::path& dir, int depth) {
        if (depth > 8) return;

        for (auto it = fs::directory_iterator(dir, ec); it != fs::end(it); it.increment(ec)) {
            if (ec) { ec.clear(); continue; }

            const auto& entry = *it;
            const auto& p = entry.path();
            auto name = p.filename().string();

            // 跳过隐藏条目
            if (name.empty() || name[0] == '.') continue;

            bool is_dir = entry.is_directory(ec);
            if (ec) { ec.clear(); continue; }

            // 跳过大型构建/缓存目录
            if (is_dir && s_skip_dirs.count(name)) continue;

            // 只添加文件, 跳过目录
            if (!is_dir) {
                bool matched = FuzzyMatch(q_lower, name);
                if (matched) {
                    std::string full = p.string();
                    std::string rel;
                    if (full.size() > base_len)
                        rel = full.substr(base_len);
                    else
                        rel = std::move(name);
                    results.push_back({std::move(rel), false});
                } else {
                    auto full = p.string();
                    if (full.size() > base_len) {
                        std::string rel = full.substr(base_len);
                        if (FuzzyMatch(q_lower, rel))
                            results.push_back({std::move(rel), false});
                    }
                }
            }

            // 递归进入子目录
            if (is_dir)
                walk(p, depth + 1);
        }
    };

    walk(base, 0);
    results.shrink_to_fit();
    return results;
}

// ---- 搜索入口: 自动选择引擎 ----
std::vector<FdResult> SearchEngine::Search(const std::string& query,
                                           const std::string& basePath) {
    if (HasExternal()) {
        return ExternalSearch(query, basePath);
    }
    return BuiltinSearch(query, basePath);
}
