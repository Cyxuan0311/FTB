#include "ops/OpenerManager.hpp"
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace FTB {

OpenerManager& OpenerManager::Instance() {
    static OpenerManager instance;
    return instance;
}

void OpenerManager::LoadConfig(const OpenerConfig& config) {
    config_ = config;
}

std::string OpenerManager::GetExtension(const std::string& filename) const {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = filename.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool OpenerManager::MatchGlob(const std::string& pattern, const std::string& filename) const {
    if (pattern == "*") return true;

    auto dot = filename.find_last_of('.');
    std::string ext = (dot != std::string::npos) ? filename.substr(dot) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    size_t brace_start = pattern.find('{');
    if (brace_start != std::string::npos && pattern.back() == '}') {
        std::string prefix = pattern.substr(0, brace_start);
        std::string content = pattern.substr(brace_start + 1);
        content.pop_back();

        std::istringstream ss(content);
        std::string token;
        while (std::getline(ss, token, ',')) {
            std::string trimmed = token;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
            if (MatchGlob(prefix + trimmed, filename)) return true;
        }
        return false;
    }

    if (pattern[0] == '*') {
        return ext == pattern.substr(1);
    }

    return false;
}

std::vector<std::pair<std::string, Opener>> OpenerManager::MatchOpeners(
    const std::string& filename) const {
    std::vector<std::pair<std::string, Opener>> result;
    std::string ext = GetExtension(filename);

    for (const auto& rule : config_.rules) {
        bool matched = false;

        if (!rule.name.empty()) {
            matched = MatchGlob(rule.name, filename);
        }

        if (!matched && !rule.mime.empty()) {
            if (rule.mime == "text/*") {
                std::vector<std::string> textExts = {
                    ".txt", ".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts",
                    ".java", ".rs", ".go", ".rb", ".php", ".html", ".css",
                    ".json", ".yaml", ".yml", ".toml", ".xml", ".md", ".sh",
                    ".bash", ".zsh", ".fish", ".vim", ".lua", ".r", ".pl",
                    ".swift", ".kt", ".scala", ".hs", ".ex", ".erl"
                };
                for (const auto& te : textExts) {
                    if (ext == te) { matched = true; break; }
                }
            }
        }

        if (matched) {
            for (const auto& openerName : rule.use) {
                auto it = config_.openers.find(openerName);
                if (it != config_.openers.end()) {
                    for (const auto& opener : it->second) {
                        result.push_back({openerName, opener});
                    }
                }
            }
            break;
        }
    }

    return result;
}

std::optional<Opener> OpenerManager::GetDefaultOpener(
    const std::string& filename) const {
    auto matched = MatchOpeners(filename);
    if (matched.empty()) return std::nullopt;
    return matched[0].second;
}

bool OpenerManager::Execute(const Opener& opener, const std::string& filepath,
                            ftxui::ScreenInteractive* screen) {
    std::string cmd = opener.run;

    if (cmd.empty()) {
        return false;
    }

    size_t pos = 0;
    while ((pos = cmd.find("%s", pos)) != std::string::npos) {
        cmd.replace(pos, 2, filepath);
        pos += filepath.length();
    }

    if (opener.orphan) {
        std::string full_cmd = cmd + " &";
        int result = std::system(full_cmd.c_str());
        return result == 0;
    }

    if (screen) {
        screen->WithRestoredIO([&]() {
            int _ = std::system(cmd.c_str());
            (void)_;
        })();
    } else {
        int _ = std::system(cmd.c_str());
        (void)_;
    }
    return true;
}

std::vector<std::string> OpenerManager::GetOpenerNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : config_.openers) {
        names.push_back(name);
    }
    return names;
}

std::optional<Opener> OpenerManager::GetOpener(const std::string& name) const {
    auto it = config_.openers.find(name);
    if (it == config_.openers.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second[0];
}

} // namespace FTB
