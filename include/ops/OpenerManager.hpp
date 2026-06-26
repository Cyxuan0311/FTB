#ifndef OPENER_MANAGER_HPP
#define OPENER_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace FTB {

struct Opener {
    std::string run;
    bool block = false;
    bool orphan = false;
    std::string desc;
    std::string platform;
};

struct OpenRule {
    std::string name;
    std::string mime;
    std::vector<std::string> use;
};

struct OpenerConfig {
    std::map<std::string, std::vector<Opener>> openers;
    std::vector<OpenRule> rules;
};

class OpenerManager {
public:
    static OpenerManager& Instance();

    void LoadConfig(const OpenerConfig& config);

    std::vector<std::pair<std::string, Opener>> MatchOpeners(
        const std::string& filename) const;

    std::optional<Opener> GetDefaultOpener(
        const std::string& filename) const;

    bool Execute(const Opener& opener, const std::string& filepath,
                 ftxui::ScreenInteractive* screen = nullptr);

    std::vector<std::string> GetOpenerNames() const;

    std::optional<Opener> GetOpener(const std::string& name) const;

private:
    OpenerManager() = default;
    bool MatchGlob(const std::string& pattern, const std::string& filename) const;
    std::string GetExtension(const std::string& filename) const;

    OpenerConfig config_;
};

} // namespace FTB

#endif // OPENER_MANAGER_HPP
