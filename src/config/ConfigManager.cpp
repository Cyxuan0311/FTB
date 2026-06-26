#include "config/ConfigManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace FTB {

std::unique_ptr<ConfigManager> ConfigManager::instance_ = nullptr;

ConfigManager::ConfigManager() : config_loaded_(false) {
    InitializePredefinedColors();
    config_path_ = GetUserHomeDir() + "/.config/ftb/ftb.json";
}

ConfigManager* ConfigManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = std::make_unique<ConfigManager>();
    }
    return instance_.get();
}

// ---- JSON 序列化辅助 ----

static json ColorConfigToJson(const ColorConfig& c) {
    return json{
        {"background", c.background}, {"foreground", c.foreground},
        {"border", c.border}, {"selection_bg", c.selection_bg}, {"selection_fg", c.selection_fg}
    };
}

static void JsonToColorConfig(const json& j, ColorConfig& c) {
    if (j.contains("background"))  j["background"].get_to(c.background);
    if (j.contains("foreground"))  j["foreground"].get_to(c.foreground);
    if (j.contains("border"))      j["border"].get_to(c.border);
    if (j.contains("selection_bg")) j["selection_bg"].get_to(c.selection_bg);
    if (j.contains("selection_fg")) j["selection_fg"].get_to(c.selection_fg);
}

static json FileTypeColorsToJson(const FileTypeColors& c) {
    return json{
        {"directory", c.directory}, {"file", c.file}, {"executable", c.executable},
        {"link", c.link}, {"hidden", c.hidden}, {"system", c.system}
    };
}

static void JsonToFileTypeColors(const json& j, FileTypeColors& c) {
    if (j.contains("directory"))   j["directory"].get_to(c.directory);
    if (j.contains("file"))        j["file"].get_to(c.file);
    if (j.contains("executable"))  j["executable"].get_to(c.executable);
    if (j.contains("link"))        j["link"].get_to(c.link);
    if (j.contains("hidden"))      j["hidden"].get_to(c.hidden);
    if (j.contains("system"))      j["system"].get_to(c.system);
}

static json StyleConfigToJson(const StyleConfig& s) {
    return json{
        {"show_icons", s.show_icons}, {"show_file_size", s.show_file_size},
        {"show_modified_time", s.show_modified_time}, {"show_permissions", s.show_permissions},
        {"enable_mouse", s.enable_mouse}, {"enable_animations", s.enable_animations},
        {"show_hidden_files", s.show_hidden_files}, {"show_detail_panel", s.show_detail_panel},
        {"sort_mode", s.sort_mode}
    };
}

static void JsonToStyleConfig(const json& j, StyleConfig& s) {
    if (j.contains("show_icons"))          j["show_icons"].get_to(s.show_icons);
    if (j.contains("show_file_size"))      j["show_file_size"].get_to(s.show_file_size);
    if (j.contains("show_modified_time"))  j["show_modified_time"].get_to(s.show_modified_time);
    if (j.contains("show_permissions"))    j["show_permissions"].get_to(s.show_permissions);
    if (j.contains("enable_mouse"))        j["enable_mouse"].get_to(s.enable_mouse);
    if (j.contains("enable_animations"))   j["enable_animations"].get_to(s.enable_animations);
    if (j.contains("show_hidden_files"))   j["show_hidden_files"].get_to(s.show_hidden_files);
    if (j.contains("show_detail_panel"))   j["show_detail_panel"].get_to(s.show_detail_panel);
    if (j.contains("sort_mode"))           j["sort_mode"].get_to(s.sort_mode);
}

static json LayoutConfigToJson(const LayoutConfig& l) {
    return json{
        {"parent_ratio", l.parent_ratio},
        {"current_ratio", l.current_ratio},
        {"preview_ratio", l.preview_ratio},
        {"items_per_page", l.items_per_page}
    };
}

static void JsonToLayoutConfig(const json& j, LayoutConfig& l) {
    // 兼容旧配置
    if (j.contains("parent_column_width")) {
        // 旧格式转换: 假设终端宽度约100
        int pw = j["parent_column_width"].get<int>();
        l.parent_ratio = pw / 100.0;
    }
    if (j.contains("detail_panel_ratio")) {
        l.preview_ratio = j["detail_panel_ratio"].get<double>();
    }
    // 新格式
    if (j.contains("parent_ratio"))  j["parent_ratio"].get_to(l.parent_ratio);
    if (j.contains("current_ratio")) j["current_ratio"].get_to(l.current_ratio);
    if (j.contains("preview_ratio")) j["preview_ratio"].get_to(l.preview_ratio);
    if (j.contains("items_per_page")) j["items_per_page"].get_to(l.items_per_page);
    l.Normalize();
}

static json UIConfigToJson(const UIConfig& u) {
    return json{
        {"column_separator", u.column_separator},
        {"panel_border", u.panel_border},
        {"selection_style", u.selection_style},
        {"tab_bar_style", u.tab_bar_style}
    };
}

static void JsonToUIConfig(const json& j, UIConfig& u) {
    if (j.contains("column_separator")) j["column_separator"].get_to(u.column_separator);
    if (j.contains("panel_border"))     j["panel_border"].get_to(u.panel_border);
    if (j.contains("selection_style"))  j["selection_style"].get_to(u.selection_style);
    if (j.contains("tab_bar_style"))    j["tab_bar_style"].get_to(u.tab_bar_style);
}

static json StatusBarConfigToJson(const StatusBarConfig& s) {
    return json{
        {"style", s.style},
        {"show_position", s.show_position},
        {"show_time", s.show_time},
        {"show_clipboard", s.show_clipboard},
        {"use_bold", s.use_bold}
    };
}

static void JsonToStatusBarConfig(const json& j, StatusBarConfig& s) {
    if (j.contains("style"))         j["style"].get_to(s.style);
    if (j.contains("show_position")) j["show_position"].get_to(s.show_position);
    if (j.contains("show_time"))     j["show_time"].get_to(s.show_time);
    if (j.contains("show_clipboard")) j["show_clipboard"].get_to(s.show_clipboard);
    if (j.contains("use_bold"))      j["use_bold"].get_to(s.use_bold);
}

static json SSHConfigToJson(const SSHConfig& s) {
    return json{
        {"default_port", s.default_port}, {"connection_timeout", s.connection_timeout},
        {"save_connection_history", s.save_connection_history},
        {"max_connection_history", s.max_connection_history}
    };
}

static void JsonToSSHConfig(const json& j, SSHConfig& s) {
    if (j.contains("default_port"))            j["default_port"].get_to(s.default_port);
    if (j.contains("connection_timeout"))       j["connection_timeout"].get_to(s.connection_timeout);
    if (j.contains("save_connection_history"))  j["save_connection_history"].get_to(s.save_connection_history);
    if (j.contains("max_connection_history"))   j["max_connection_history"].get_to(s.max_connection_history);
}

static json LoggingConfigToJson(const LoggingConfig& l) {
    return json{
        {"level", l.level}, {"output_to_file", l.output_to_file},
        {"log_file", l.log_file}, {"show_timestamp", l.show_timestamp}
    };
}

static void JsonToLoggingConfig(const json& j, LoggingConfig& l) {
    if (j.contains("level"))           j["level"].get_to(l.level);
    if (j.contains("output_to_file"))  j["output_to_file"].get_to(l.output_to_file);
    if (j.contains("log_file"))        j["log_file"].get_to(l.log_file);
    if (j.contains("show_timestamp"))  j["show_timestamp"].get_to(l.show_timestamp);
}

static json PreviewConfigToJson(const PreviewConfig& p) {
    return json{
        {"max_text_file_size_kb", p.max_text_file_size_kb},
        {"max_text_lines", p.max_text_lines},
        {"max_dir_entries", p.max_dir_entries},
        {"max_hex_bytes", p.max_hex_bytes},
        {"max_archive_nodes", p.max_archive_nodes},
        {"max_spreadsheet_rows", p.max_spreadsheet_rows},
        {"truncate_long_lines", p.truncate_long_lines},
        {"chunk_size_lines", p.chunk_size_lines},
        {"virtual_scroll_margin", p.virtual_scroll_margin}
    };
}

static void JsonToPreviewConfig(const json& j, PreviewConfig& p) {
    if (j.contains("max_text_file_size_kb"))  j["max_text_file_size_kb"].get_to(p.max_text_file_size_kb);
    if (j.contains("max_text_lines"))         j["max_text_lines"].get_to(p.max_text_lines);
    if (j.contains("max_dir_entries"))        j["max_dir_entries"].get_to(p.max_dir_entries);
    if (j.contains("max_hex_bytes"))          j["max_hex_bytes"].get_to(p.max_hex_bytes);
    if (j.contains("max_archive_nodes"))      j["max_archive_nodes"].get_to(p.max_archive_nodes);
    if (j.contains("max_spreadsheet_rows"))   j["max_spreadsheet_rows"].get_to(p.max_spreadsheet_rows);
    if (j.contains("truncate_long_lines"))    j["truncate_long_lines"].get_to(p.truncate_long_lines);
    if (j.contains("chunk_size_lines"))       j["chunk_size_lines"].get_to(p.chunk_size_lines);
    if (j.contains("virtual_scroll_margin"))  j["virtual_scroll_margin"].get_to(p.virtual_scroll_margin);
}

static json BookmarkConfigToJson(const BookmarkConfig& b) {
    return json(b.bookmarks);
}

static void JsonToBookmarkConfig(const json& j, BookmarkConfig& b) {
    if (j.is_object()) {
        for (auto& [key, val] : j.items()) {
            if (val.is_string()) {
                b.bookmarks[key] = val.get<std::string>();
            }
        }
    }
}

static json SSHRecordToJson(const SSHRecord& r) {
    return json{
        {"host", r.host},
        {"port", r.port},
        {"username", r.username},
        {"remote_directory", r.remote_directory}
    };
}

static SSHRecord JsonToSSHRecord(const json& j) {
    SSHRecord r;
    if (j.contains("host"))             j["host"].get_to(r.host);
    if (j.contains("port"))             j["port"].get_to(r.port);
    if (j.contains("username"))         j["username"].get_to(r.username);
    if (j.contains("remote_directory")) j["remote_directory"].get_to(r.remote_directory);
    return r;
}

// ---- 配置文件读写 ----

bool ConfigManager::LoadConfig(const std::string& config_path) {
    if (!config_path.empty()) {
        config_path_ = config_path;
    }

    if (!fs::exists(config_path_)) {
        if (!CreateDefaultConfig()) {
            std::cerr << "Warning: Cannot create default config file" << std::endl;
            config_ = GetDefaultConfig();
            config_loaded_ = true;
            return false;
        }
    }

    std::ifstream file(config_path_);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open config file: " << config_path_ << std::endl;
        config_ = GetDefaultConfig();
        config_loaded_ = true;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    if (!ParseConfigFile(buffer.str())) {
        std::cerr << "Warning: Config parse failed, using defaults" << std::endl;
        config_ = GetDefaultConfig();
        config_loaded_ = true;
        return false;
    }

    LoadNoPreviewConfig();

    config_loaded_ = true;
    ApplyColorConfig();
    return true;
}

bool ConfigManager::SaveConfig(const std::string& config_path) {
    std::string save_path = config_path.empty() ? config_path_ : config_path;

    fs::create_directories(fs::path(save_path).parent_path());

    std::ofstream file(save_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create config file: " << save_path << std::endl;
        return false;
    }

    json root;
    root["colors"] = {
        {"main", ColorConfigToJson(config_.colors_main)},
        {"files", FileTypeColorsToJson(config_.colors_files)},
        {"status", ColorConfigToJson(config_.colors_status)},
        {"search", ColorConfigToJson(config_.colors_search)},
        {"dialog", ColorConfigToJson(config_.colors_dialog)}
    };
    root["style"]     = StyleConfigToJson(config_.style);
    root["layout"]    = LayoutConfigToJson(config_.layout);
    root["ui"]        = UIConfigToJson(config_.ui);
    root["statusbar"] = StatusBarConfigToJson(config_.statusbar);
    root["refresh"] = json{
        {"ui_refresh_interval_ms", config_.refresh.ui_refresh_interval_ms},
        {"content_refresh_interval_ms", config_.refresh.content_refresh_interval_ms},
        {"auto_refresh", config_.refresh.auto_refresh}
    };
    root["theme"]   = json{
        {"name", config_.theme.name},
        {"use_bold", config_.theme.use_bold},
        {"use_underline", config_.theme.use_underline}
    };
    root["ssh"]     = SSHConfigToJson(config_.ssh);
    root["logging"] = LoggingConfigToJson(config_.logging);
    root["bookmarks"] = BookmarkConfigToJson(config_.bookmarks);
    root["preview"] = PreviewConfigToJson(config_.preview);

    json ssh_records_json = json::array();
    for (const auto& rec : ssh_records_) {
        ssh_records_json.push_back(SSHRecordToJson(rec));
    }
    root["ssh_records"] = ssh_records_json;

    file << root.dump(2) << std::endl;
    file.close();
    return true;
}

bool ConfigManager::ParseConfigFile(const std::string& content) {
    try {
        json root = json::parse(content);

        if (root.contains("colors")) {
            auto& c = root["colors"];
            if (c.contains("main"))   JsonToColorConfig(c["main"], config_.colors_main);
            if (c.contains("files"))  JsonToFileTypeColors(c["files"], config_.colors_files);
            if (c.contains("status")) JsonToColorConfig(c["status"], config_.colors_status);
            if (c.contains("search")) JsonToColorConfig(c["search"], config_.colors_search);
            if (c.contains("dialog")) JsonToColorConfig(c["dialog"], config_.colors_dialog);
        }
        if (root.contains("style"))   JsonToStyleConfig(root["style"], config_.style);
        if (root.contains("layout"))  JsonToLayoutConfig(root["layout"], config_.layout);
        if (root.contains("ui"))      JsonToUIConfig(root["ui"], config_.ui);
        if (root.contains("statusbar")) JsonToStatusBarConfig(root["statusbar"], config_.statusbar);
        if (root.contains("refresh")) {
            auto& r = root["refresh"];
            if (r.contains("ui_refresh_interval_ms"))      r["ui_refresh_interval_ms"].get_to(config_.refresh.ui_refresh_interval_ms);
            if (r.contains("content_refresh_interval_ms"))  r["content_refresh_interval_ms"].get_to(config_.refresh.content_refresh_interval_ms);
            if (r.contains("auto_refresh"))                 r["auto_refresh"].get_to(config_.refresh.auto_refresh);
        }
        if (root.contains("theme")) {
            auto& t = root["theme"];
            if (t.contains("name"))          t["name"].get_to(config_.theme.name);
            if (t.contains("use_bold"))      t["use_bold"].get_to(config_.theme.use_bold);
            if (t.contains("use_underline")) t["use_underline"].get_to(config_.theme.use_underline);
        }
        if (root.contains("ssh"))     JsonToSSHConfig(root["ssh"], config_.ssh);
        if (root.contains("logging")) JsonToLoggingConfig(root["logging"], config_.logging);
        if (root.contains("bookmarks")) JsonToBookmarkConfig(root["bookmarks"], config_.bookmarks);

        if (root.contains("opener")) {
            auto& o = root["opener"];
            if (o.contains("openers") && o["openers"].is_object()) {
                for (auto& [name, arr] : o["openers"].items()) {
                    std::vector<Opener> openers_list;
                    if (arr.is_array()) {
                        for (const auto& item : arr) {
                            Opener op;
                            if (item.contains("run"))     op.run = item["run"].get<std::string>();
                            if (item.contains("block"))   op.block = item["block"].get<bool>();
                            if (item.contains("orphan"))  op.orphan = item["orphan"].get<bool>();
                            if (item.contains("desc"))    op.desc = item["desc"].get<std::string>();
                            if (item.contains("platform")) op.platform = item["platform"].get<std::string>();
                            openers_list.push_back(op);
                        }
                    }
                    config_.opener.openers[name] = openers_list;
                }
            }
            if (o.contains("rules") && o["rules"].is_array()) {
                for (const auto& rule : o["rules"]) {
                    OpenRule r;
                    if (rule.contains("name")) r.name = rule["name"].get<std::string>();
                    if (rule.contains("mime")) r.mime = rule["mime"].get<std::string>();
                    if (rule.contains("use") && rule["use"].is_array()) {
                        for (const auto& u : rule["use"]) {
                            r.use.push_back(u.get<std::string>());
                        }
                    }
                    config_.opener.rules.push_back(r);
                }
            }
        }

        if (root.contains("ssh_records") && root["ssh_records"].is_array()) {
            ssh_records_.clear();
            for (const auto& item : root["ssh_records"]) {
                ssh_records_.push_back(JsonToSSHRecord(item));
            }
        }

        if (root.contains("preview")) {
            JsonToPreviewConfig(root["preview"], config_.preview);
        }

    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    } catch (const json::type_error& e) {
        std::cerr << "JSON type error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// ---- 颜色操作 ----

ftxui::Color ConfigManager::GetColor(const std::string& color_name) const {
    auto it = predefined_colors_.find(color_name);
    if (it != predefined_colors_.end()) return it->second;
    auto custom_it = config_.custom_colors.find(color_name);
    if (custom_it != config_.custom_colors.end()) return custom_it->second;
    return ftxui::Color::White;
}

ftxui::Color ConfigManager::GetFileTypeColor(const std::string& file_type) const {
    if (file_type == "directory")   return ParseColor(config_.colors_files.directory);
    if (file_type == "executable") return ParseColor(config_.colors_files.executable);
    if (file_type == "link")       return ParseColor(config_.colors_files.link);
    if (file_type == "hidden")     return ParseColor(config_.colors_files.hidden);
    if (file_type == "system")     return ParseColor(config_.colors_files.system);
    return ParseColor(config_.colors_files.file);
}

void ConfigManager::ApplyTheme(const std::string& theme_name) {
    config_.theme.name = theme_name;
    ApplyColorConfig();
}

bool ConfigManager::ReloadConfig() { return LoadConfig(); }

FTBConfig ConfigManager::GetDefaultConfig() const { return FTBConfig{}; }

void ConfigManager::ResetToDefault() {
    config_ = GetDefaultConfig();
    ApplyColorConfig();
}

bool ConfigManager::CreateDefaultConfig() {
    config_ = GetDefaultConfig();
    return SaveConfig();
}

std::string ConfigManager::GetUserHomeDir() const {
    const char* home = std::getenv("HOME");
    if (home) return std::string(home);
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) return std::string(userProfile);
#endif
    return std::string();
}

bool ConfigManager::ValidateConfig() const {
    if (config_.layout.parent_ratio < 0.0 || config_.layout.current_ratio < 0.0 || config_.layout.preview_ratio < 0.0)
        return false;
    if (config_.refresh.ui_refresh_interval_ms <= 0)
        return false;
    return true;
}

void ConfigManager::LoadNoPreviewConfig() {
    no_preview_extensions_.clear();

    std::string noavail_path = fs::path(config_path_).parent_path().string() + "/noavilable.json";

    if (!fs::exists(noavail_path)) {
        fs::create_directories(fs::path(noavail_path).parent_path());
        std::ofstream file(noavail_path);
        if (file.is_open()) {
            json j;
            j["no_preview_extensions"] = json::array();
            file << j.dump(2) << std::endl;
        }
        return;
    }

    std::ifstream file(noavail_path);
    if (!file.is_open()) return;

    try {
        json j;
        file >> j;
        if (j.contains("no_preview_extensions") && j["no_preview_extensions"].is_array()) {
            for (const auto& ext : j["no_preview_extensions"]) {
                if (ext.is_string()) {
                    no_preview_extensions_.insert(ext.get<std::string>());
                }
            }
        }
    } catch (...) {}
}

bool ConfigManager::IsNoPreviewExtension(const std::string& ext) const {
    return no_preview_extensions_.find(ext) != no_preview_extensions_.end();
}

void ConfigManager::ApplyColorConfig() {
    config_.custom_colors["main_bg"]      = ParseColor(config_.colors_main.background);
    config_.custom_colors["main_fg"]      = ParseColor(config_.colors_main.foreground);
    config_.custom_colors["main_border"]  = ParseColor(config_.colors_main.border);
    config_.custom_colors["selection_bg"] = ParseColor(config_.colors_main.selection_bg);
    config_.custom_colors["selection_fg"] = ParseColor(config_.colors_main.selection_fg);
}

ftxui::Color ConfigManager::ParseColor(const std::string& color_str) const {
    // Hex color: #RRGGBB
    if (color_str.size() == 7 && color_str[0] == '#') {
        try {
            uint8_t r = static_cast<uint8_t>(std::stoul(color_str.substr(1, 2), nullptr, 16));
            uint8_t g = static_cast<uint8_t>(std::stoul(color_str.substr(3, 2), nullptr, 16));
            uint8_t b = static_cast<uint8_t>(std::stoul(color_str.substr(5, 2), nullptr, 16));
            return ftxui::Color::RGB(r, g, b);
        } catch (...) {}
    }
    // Named color
    auto it = predefined_colors_.find(color_str);
    if (it != predefined_colors_.end()) return it->second;
    return ftxui::Color::White;
}

void ConfigManager::InitializePredefinedColors() {
    predefined_colors_["black"]         = ftxui::Color::Black;
    predefined_colors_["red"]           = ftxui::Color::Red;
    predefined_colors_["green"]         = ftxui::Color::Green;
    predefined_colors_["yellow"]        = ftxui::Color::Yellow;
    predefined_colors_["blue"]          = ftxui::Color::Blue;
    predefined_colors_["magenta"]       = ftxui::Color::Magenta;
    predefined_colors_["cyan"]          = ftxui::Color::Cyan;
    predefined_colors_["white"]         = ftxui::Color::White;
    predefined_colors_["bright_black"]  = ftxui::Color::GrayDark;
    predefined_colors_["bright_red"]    = ftxui::Color::RedLight;
    predefined_colors_["bright_green"]  = ftxui::Color::GreenLight;
    predefined_colors_["bright_yellow"] = ftxui::Color::YellowLight;
    predefined_colors_["bright_blue"]   = ftxui::Color::BlueLight;
    predefined_colors_["bright_magenta"]= ftxui::Color::MagentaLight;
    predefined_colors_["bright_cyan"]   = ftxui::Color::CyanLight;
    predefined_colors_["bright_white"]  = ftxui::Color::GrayLight;
}

// ---- 书签操作 ----

bool ConfigManager::AddBookmark(const std::string& name, const std::string& path) {
    config_.bookmarks.bookmarks[name] = path;
    return SaveConfig();
}

bool ConfigManager::RemoveBookmark(const std::string& name) {
    auto it = config_.bookmarks.bookmarks.find(name);
    if (it == config_.bookmarks.bookmarks.end()) return false;
    config_.bookmarks.bookmarks.erase(it);
    return SaveConfig();
}

std::vector<std::pair<std::string, std::string>> ConfigManager::GetBookmarks() const {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& [name, path] : config_.bookmarks.bookmarks) {
        result.emplace_back(name, path);
    }
    return result;
}

// ---- SSH 记录操作 ----

std::vector<SSHRecord> ConfigManager::GetSSHRecords() const {
    return ssh_records_;
}

void ConfigManager::AddSSHRecord(const SSHRecord& record) {
    // 去重: 相同 host+port+username 不重复添加
    for (auto& rec : ssh_records_) {
        if (rec.host == record.host && rec.port == record.port && rec.username == record.username) {
            rec.remote_directory = record.remote_directory;
            SaveConfig();
            return;
        }
    }
    ssh_records_.push_back(record);
    // 限制数量
    if (config_.ssh.max_connection_history > 0 &&
        static_cast<int>(ssh_records_.size()) > config_.ssh.max_connection_history) {
        ssh_records_.erase(ssh_records_.begin());
    }
    SaveConfig();
}

void ConfigManager::RemoveSSHRecord(int index) {
    if (index >= 0 && index < static_cast<int>(ssh_records_.size())) {
        ssh_records_.erase(ssh_records_.begin() + index);
        SaveConfig();
    }
}

void ConfigManager::SetSSHRecords(const std::vector<SSHRecord>& records) {
    ssh_records_ = records;
    SaveConfig();
}

} // namespace FTB
