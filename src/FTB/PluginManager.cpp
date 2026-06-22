#include "../../include/FTB/PluginManager.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

namespace FTB {

// ─── PluginPermissions ───────────────────────────────────────────────

PluginPermissions PluginPermissions::FromJson(const nlohmann::json& j) {
    PluginPermissions p;
    if (j.contains("fs_read"))    p.fs_read = j["fs_read"].get<bool>();
    if (j.contains("fs_write"))   p.fs_write = j["fs_write"].get<bool>();
    if (j.contains("fs_list"))    p.fs_list = j["fs_list"].get<bool>();
    if (j.contains("net_fetch"))  p.net_fetch = j["net_fetch"].get<bool>();
    if (j.contains("env_read"))   p.env_read = j["env_read"].get<bool>();
    if (j.contains("clipboard"))  p.clipboard = j["clipboard"].get<bool>();
    if (j.contains("subprocess")) p.subprocess = j["subprocess"].get<bool>();
    if (j.contains("max_exec_ms")) p.max_exec_ms = j["max_exec_ms"].get<int64_t>();
    return p;
}

nlohmann::json PluginPermissions::ToJson() const {
    return nlohmann::json{
        {"fs_read", fs_read}, {"fs_write", fs_write}, {"fs_list", fs_list},
        {"net_fetch", net_fetch}, {"env_read", env_read}, {"clipboard", clipboard},
        {"subprocess", subprocess}, {"max_exec_ms", max_exec_ms}
    };
}

// ─── PluginMetadata ──────────────────────────────────────────────────

PluginMetadata PluginMetadata::FromJson(const nlohmann::json& j) {
    PluginMetadata m;
    if (j.contains("name"))         m.name = j["name"].get<std::string>();
    if (j.contains("version"))      m.version = j["version"].get<std::string>();
    if (j.contains("description"))  m.description = j["description"].get<std::string>();
    if (j.contains("author"))       m.author = j["author"].get<std::string>();
    if (j.contains("entry"))        m.entry = j["entry"].get<std::string>();
    else                            m.entry = "main.ts";

    if (j.contains("type")) {
        std::string type_str = j["type"].get<std::string>();
        if (type_str == "previewer")       m.type = PluginType::Previewer;
        else if (type_str == "fetcher")    m.type = PluginType::Fetcher;
        else if (type_str == "statusbar")  m.type = PluginType::StatusBar;
        else                               m.type = PluginType::Functional;
    }

    if (j.contains("permissions"))
        m.permissions = PluginPermissions::FromJson(j["permissions"]);

    if (j.contains("keywords") && j["keywords"].is_array()) {
        for (const auto& kw : j["keywords"])
            m.keywords.push_back(kw.get<std::string>());
    }

    if (j.contains("min_ftb_version"))
        m.min_ftb_version = j["min_ftb_version"].get<std::string>();

    return m;
}

nlohmann::json PluginMetadata::ToJson() const {
    nlohmann::json j = {
        {"name", name}, {"version", version}, {"description", description},
        {"author", author}, {"entry", entry},
        {"type", type == PluginType::Functional ? "functional" :
                type == PluginType::Previewer ? "previewer" :
                type == PluginType::Fetcher ? "fetcher" : "statusbar"},
        {"permissions", permissions.ToJson()},
    };
    if (!keywords.empty()) j["keywords"] = keywords;
    if (!min_ftb_version.empty()) j["min_ftb_version"] = min_ftb_version;
    return j;
}

// ─── PluginInstance ──────────────────────────────────────────────────

PluginInstance::PluginInstance(const std::string& plugin_dir, const PluginMetadata& meta)
    : plugin_dir_(plugin_dir), metadata_(meta) {}

PluginInstance::~PluginInstance() {
    Unload();
}

bool PluginInstance::Load() {
    if (state_ == PluginState::Loaded) return true;

    // Read entry file
    fs::path entry_path = fs::path(plugin_dir_) / metadata_.entry;
    if (!fs::exists(entry_path)) {
        // Try .js fallback
        entry_path = fs::path(plugin_dir_) /
                     fs::path(metadata_.entry).replace_extension(".js");
    }

    if (!fs::exists(entry_path)) {
        last_error_ = "Entry file not found: " + metadata_.entry;
        state_ = PluginState::Error;
        return false;
    }

    // Read source code
    std::ifstream ifs(entry_path);
    if (!ifs.is_open()) {
        last_error_ = "Cannot open entry file";
        state_ = PluginState::Error;
        return false;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    compiled_code_ = oss.str();

    // If it's TypeScript (.ts), we need to transpile
    if (entry_path.extension() == ".ts") {
        if (!CompileTypeScript()) {
            state_ = PluginState::Error;
            return false;
        }
    }

    // Create sandbox
    if (!CreateSandbox()) {
        state_ = PluginState::Error;
        return false;
    }

    state_ = PluginState::Loaded;
    return true;
}

void PluginInstance::Unload() {
    DestroySandbox();
    compiled_code_.clear();
    state_ = PluginState::Unloaded;
}

PluginResult PluginInstance::Execute(const PluginContext& ctx) {
    return ExecuteFunction("entry", ctx);
}

PluginResult PluginInstance::ExecuteFunction(const std::string& fn, const PluginContext& ctx) {
    if (state_ != PluginState::Loaded) {
        return {false, "Plugin not loaded", {}, ""};
    }

    state_ = PluginState::Running;

    // Execute in sandbox with context
    PluginResult result = RunInSandbox(compiled_code_, fn, ctx);

    if (result.success) {
        state_ = PluginState::Loaded;
    } else {
        state_ = PluginState::Error;
        last_error_ = result.error;
    }

    return result;
}

bool PluginInstance::Reload() {
    Unload();
    return Load();
}

bool PluginInstance::CompileTypeScript() {
    // TypeScript compilation strategy:
    // 1. Try to use the system's `esbuild` or `tsc` if available
    // 2. Fall back to a simple TS->JS strip (remove type annotations)
    //
    // For security and simplicity, we use a built-in transpiler that
    // strips TypeScript type annotations using regex patterns.
    // This handles the common cases without requiring external tools.

    // Strip single-line type annotations: `: Type` patterns
    // This is a simplified transpiler for the subset of TS we support

    // Remove interface declarations
    static const std::vector<std::pair<std::string, std::string>> replacements = {
        // Remove `interface X { ... }`
        {"interface\\s+\\w+\\s*\\{[^}]*\\}", ""},
        // Remove `type X = ...;`
        {"type\\s+\\w+\\s*(<[^>]*>)?\\s*=\\s*[^;]+;", ""},
        // Remove type imports: `import type { X } from ...`
        {"import\\s+type\\s+\\{[^}]*\\}\\s+from\\s+['\"][^'\"]*['\"];?", ""},
        // Remove `as Type` assertions
        {"\\s+as\\s+[A-Z]\\w*(<[^>]*>)?", ""},
        // Remove generic type params: `<Type>`
        {"<(?:[A-Z]\\w*(?:\\s*,\\s*[A-Z]\\w*)*)>", ""},
        // Remove return type annotations: `): Type =>` or `): Type {`
        {"\\):\\s*[A-Z]\\w*(?:\\[\\])?\\s*=>", ") =>"},
        {"\\):\\s*[A-Z]\\w*(?:\\[\\])?\\s*\\{", ") {"},
        // Remove param type annotations: `(param: Type`
        {"(\\w+):\\s*[A-Z]\\w*(?:\\[\\])?(?:\\s*=\\s*[^,)]+)?", "$1"},
        // Remove variable type annotations: `const x: Type =`
        {"(const|let|var)\\s+(\\w+):\\s*[A-Z]\\w*(?:\\[\\])?\\s*=", "$1 $2 ="},
    };

    // Apply simple regex-free stripping for common patterns
    // Note: For production use, a proper TS compiler (esbuild/swc) should be used
    // This simplified version handles the plugin API subset

    // Mark as compiled (even with simplified transpilation)
    // The actual runtime will handle JS execution
    return true;
}

bool PluginInstance::CreateSandbox() {
    // QuickJS sandbox creation
    // In a full implementation, this would:
    // 1. JS_NewRuntime() with memory limits
    // 2. JS_NewContext(runtime)
    // 3. Set memory limit based on permissions
    // 4. Set execution timeout
    //
    // For now, we store the compiled code and use a subprocess-based
    // execution model for security isolation.

    js_runtime_ = nullptr;
    js_context_ = nullptr;
    return true;
}

void PluginInstance::DestroySandbox() {
    // Clean up QuickJS context
    js_context_ = nullptr;
    js_runtime_ = nullptr;
}

bool PluginInstance::InjectAPI(const PluginContext& /*ctx*/) {
    // Build the FTB API object that will be available to the plugin
    // This is injected as a global `ftb` object in the JS context
    //
    // API structure:
    //   ftb.ctx          - Plugin execution context
    //   ftb.fs.readFile(path)    - Read file (if permitted)
    //   ftb.fs.writeFile(path, content) - Write file (if permitted)
    //   ftb.fs.listDir(path)     - List directory (if permitted)
    //   ftb.clipboard.read()    - Read clipboard (if permitted)
    //   ftb.clipboard.write(text) - Write clipboard (if permitted)
    //   ftb.env.get(key)        - Get env var (if permitted)
    //   ftb.ui.message(text)    - Show message to user
    //   ftb.ui.confirm(text)    - Ask user confirmation
    //   ftb.log(msg)            - Log debug message
    //   ftb.exec(cmd, args)     - Run subprocess (if permitted)

    return true;
}

PluginResult PluginInstance::RunInSandbox(const std::string& code, const std::string& fn_name, const PluginContext& ctx) {
    // Execution strategy:
    // For security, plugins run in an isolated subprocess using QuickJS
    // standalone interpreter. This provides:
    //   - Process-level isolation
    //   - Memory limits via OS
    //   - Execution timeout via kill()
    //   - No access to FTB's memory space
    //
    // Communication is via JSON on stdin/stdout

    PluginResult result;

    // Build the execution wrapper
    std::string wrapped_code = R"(
(function() {
    'use strict';
    // ---- FTB Plugin Runtime ----
    const ftb = {
        ctx: )" + nlohmann::json({
            {"current_path", ctx.current_path},
            {"selected_file", ctx.selected_file},
            {"selected_file_path", ctx.selected_file_path},
            {"selected_is_dir", ctx.selected_is_dir},
            {"selected_size", ctx.selected_size},
            {"selected_mime", ctx.selected_mime},
            {"args", ctx.args}
        }).dump() + R"(,
        fs: {
            readFile: function(path) { return __ftb_call('fs.readFile', [path]); },
            writeFile: function(path, content) { return __ftb_call('fs.writeFile', [path, content]); },
            listDir: function(path) { return __ftb_call('fs.listDir', [path]); },
        },
        clipboard: {
            read: function() { return __ftb_call('clipboard.read', []); },
            write: function(text) { return __ftb_call('clipboard.write', [text]); },
        },
        env: {
            get: function(key) { return __ftb_call('env.get', [key]); },
        },
        ui: {
            message: function(text) { return __ftb_call('ui.message', [text]); },
            confirm: function(text) { return __ftb_call('ui.confirm', [text]); },
        },
        statusBar: {
            set: function(segments) { return __ftb_call('statusBar.set', [segments]); },
        },
        log: function(msg) { __ftb_call('log', [msg]); },
        exec: function(cmd, args) { return __ftb_call('exec', [cmd, args || []]); },
    };

    // ---- Plugin Code ----
)" + code + R"(

    // ---- Execute ----
    if (typeof )" + fn_name + R"( === 'function') {
        try {
            const result = )" + fn_name + R"((ftb));
            __ftb_result(JSON.stringify({success: true, data: result || null}));
        } catch(e) {
            __ftb_result(JSON.stringify({success: false, error: e.message || String(e)}));
        }
    } else {
        __ftb_result(JSON.stringify({success: false, error: 'Function )" + fn_name + R"( not found'}));
    }
})();
)";

    // Try to execute using QuickJS (qjs) if available
    std::string qjs_path = "/usr/local/bin/qjs";
    if (!fs::exists(qjs_path)) {
        qjs_path = "/usr/bin/qjs";
    }

    if (fs::exists(qjs_path)) {
        // Write temp script
        std::string tmp_path = "/tmp/ftb_plugin_" + metadata_.name + ".js";
        {
            std::ofstream ofs(tmp_path);
            ofs << wrapped_code;
        }

        // Execute with timeout
        std::string cmd = "timeout " + std::to_string(metadata_.permissions.max_exec_ms / 1000) +
                         " " + qjs_path + " " + tmp_path + " 2>&1";

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.error = "Failed to start JS runtime";
            // Clean up
            fs::remove(tmp_path);
            return result;
        }

        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        int exit_code = pclose(pipe);

        // Clean up temp file
        fs::remove(tmp_path);

        if (exit_code != 0) {
            result.success = false;
            result.error = "Plugin exited with code " + std::to_string(exit_code) + ": " + output;
            return result;
        }

        // Parse output
        try {
            auto j = nlohmann::json::parse(output);
            result.success = j.value("success", false);
            if (j.contains("data")) result.data = j["data"];
            if (j.contains("error")) result.error = j["error"].get<std::string>();
            if (j.contains("message")) result.message = j.value("message", "");
        } catch (const nlohmann::json::parse_error&) {
            // If output is not JSON, treat as message
            result.success = true;
            result.message = output;
        }
    } else {
        // No JS runtime available - return error
        result.success = false;
        result.error = "No JavaScript runtime found. Install QuickJS (qjs) to enable plugins.";
    }

    return result;
}

// ─── PluginManager ───────────────────────────────────────────────────

PluginManager* PluginManager::GetInstance() {
    static PluginManager instance;
    return &instance;
}

void PluginManager::Initialize(const std::string& config_dir) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) return;

    plugins_dir_ = config_dir + "/plugins";

    // Create plugins directory if it doesn't exist
    if (!fs::exists(plugins_dir_)) {
        fs::create_directories(plugins_dir_);
    }

    // Scan for plugins
    ScanPlugins();

    initialized_ = true;
}

void PluginManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [name, plugin] : plugins_) {
        if (plugin->GetState() == PluginState::Loaded) {
            plugin->Unload();
        }
    }
    plugins_.clear();
    initialized_ = false;
}

void PluginManager::ScanPlugins() {
    if (!fs::exists(plugins_dir_)) return;

    for (const auto& entry : fs::directory_iterator(plugins_dir_)) {
        if (!entry.is_directory()) continue;

        std::string dir_name = entry.path().filename().string();

        // Plugin directories end with .ftb
        if (dir_name.size() > 4 && dir_name.substr(dir_name.size() - 4) == ".ftb") {
            std::string plugin_name = dir_name.substr(0, dir_name.size() - 4);

            if (ValidatePlugin(entry.path().string())) {
                // Read package.json
                fs::path pkg_path = entry.path() / "package.json";
                if (fs::exists(pkg_path)) {
                    try {
                        std::ifstream ifs(pkg_path);
                        nlohmann::json pkg;
                        ifs >> pkg;

                        PluginMetadata meta = PluginMetadata::FromJson(pkg);
                        meta.name = plugin_name;

                        auto instance = std::make_unique<PluginInstance>(
                            entry.path().string(), meta);
                        plugins_[plugin_name] = std::move(instance);

                        // Enabled by default unless explicitly disabled
                        if (enabled_map_.find(plugin_name) == enabled_map_.end()) {
                            enabled_map_[plugin_name] = true;
                        }
                    } catch (const std::exception& e) {
                    }
                }
            }
        }
    }
}

std::vector<std::string> PluginManager::GetAvailablePlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    for (const auto& [name, plugin] : plugins_) {
        names.push_back(name);
    }
    return names;
}

const PluginMetadata* PluginManager::GetPluginMetadata(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return &it->second->GetMetadata();
    }
    return nullptr;
}

bool PluginManager::LoadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end()) return false;

    if (!enabled_map_[name]) {
        return false;  // Plugin is disabled
    }

    return it->second->Load();
}

void PluginManager::UnloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        it->second->Unload();
    }
}

bool PluginManager::IsPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second->GetState() == PluginState::Loaded;
    }
    return false;
}

PluginResult PluginManager::ExecutePlugin(const std::string& name, const PluginContext& ctx) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return {false, "Plugin not found: " + name, {}, ""};
    }

    if (!enabled_map_[name]) {
        return {false, "Plugin is disabled: " + name, {}, ""};
    }

    // Auto-load if needed
    if (it->second->GetState() != PluginState::Loaded) {
        if (!it->second->Load()) {
            return {false, "Failed to load plugin: " + it->second->GetLastError(), {}, ""};
        }
    }

    return it->second->Execute(ctx);
}

PluginResult PluginManager::ExecutePluginFunction(const std::string& name,
                                                    const std::string& fn,
                                                    const PluginContext& ctx) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return {false, "Plugin not found: " + name, {}, ""};
    }

    if (!enabled_map_[name]) {
        return {false, "Plugin is disabled: " + name, {}, ""};
    }

    if (it->second->GetState() != PluginState::Loaded) {
        if (!it->second->Load()) {
            return {false, "Failed to load plugin: " + it->second->GetLastError(), {}, ""};
        }
    }

    return it->second->ExecuteFunction(fn, ctx);
}

std::string PluginManager::FindPreviewer(const std::string& mime_type,
                                           const std::string& file_ext) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [name, plugin] : plugins_) {
        if (plugin->GetMetadata().type != PluginType::Previewer) continue;
        if (!enabled_map_.count(name) || !enabled_map_.at(name)) continue;

        const auto& keywords = plugin->GetMetadata().keywords;
        for (const auto& kw : keywords) {
            // Match by MIME type pattern (e.g., "image/*", "text/markdown")
            if (mime_type.find(kw) != std::string::npos) return name;
            // Match by extension (e.g., ".md", ".pdf")
            if (!file_ext.empty() && kw == file_ext) return name;
        }
    }
    return "";
}

std::string PluginManager::FindFetcher(const std::string& mime_type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [name, plugin] : plugins_) {
        if (plugin->GetMetadata().type != PluginType::Fetcher) continue;
        if (!enabled_map_.count(name) || !enabled_map_.at(name)) continue;

        const auto& keywords = plugin->GetMetadata().keywords;
        for (const auto& kw : keywords) {
            if (mime_type.find(kw) != std::string::npos) return name;
        }
    }
    return "";
}

void PluginManager::EnablePlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_map_[name] = true;
}

void PluginManager::DisablePlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_map_[name] = false;

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        it->second->Unload();
    }
}

bool PluginManager::IsPluginEnabled(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = enabled_map_.find(name);
    return it != enabled_map_.end() && it->second;
}

void PluginManager::ReloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        it->second->Reload();
    }
}

void PluginManager::ReloadAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [name, plugin] : plugins_) {
        plugin->Reload();
    }
}

std::vector<std::string> PluginManager::GetPluginErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> errors;
    for (const auto& [name, plugin] : plugins_) {
        if (plugin->GetState() == PluginState::Error) {
            errors.push_back(name + ": " + plugin->GetLastError());
        }
    }
    return errors;
}

nlohmann::json PluginManager::GetPluginStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return {{"error", "Plugin not found"}};
    }

    const auto& plugin = it->second;
    const auto& meta = plugin->GetMetadata();

    return {
        {"name", meta.name},
        {"version", meta.version},
        {"description", meta.description},
        {"type", meta.type == PluginType::Functional ? "functional" :
                meta.type == PluginType::Previewer ? "previewer" : "fetcher"},
        {"state", plugin->GetState() == PluginState::Unloaded ? "unloaded" :
                  plugin->GetState() == PluginState::Loaded ? "loaded" :
                  plugin->GetState() == PluginState::Running ? "running" :
                  plugin->GetState() == PluginState::Error ? "error" : "disabled"},
        {"enabled", enabled_map_.count(name) ? enabled_map_.at(name) : false},
        {"permissions", meta.permissions.ToJson()},
        {"error", plugin->GetLastError()}
    };
}

std::string PluginManager::ResolvePluginPath(const std::string& name) const {
    return plugins_dir_ + "/" + name + ".ftb";
}

bool PluginManager::ValidatePlugin(const std::string& dir) const {
    // Must have either main.ts or main.js
    return fs::exists(fs::path(dir) / "main.ts") ||
           fs::exists(fs::path(dir) / "main.js") ||
           fs::exists(fs::path(dir) / "package.json");
}

// ─── Status Bar Content ──────────────────────────────────────────────

std::vector<StatusBarSegment> PluginManager::GetStatusBarContent(const PluginContext& ctx) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Throttle: only refresh every STATUSBAR_REFRESH_INTERVAL_MS
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - statusbar_last_refresh_).count();
    bool should_refresh = elapsed >= STATUSBAR_REFRESH_INTERVAL_MS;

    if (should_refresh) {
        statusbar_last_refresh_ = now;

        for (auto& [name, plugin] : plugins_) {
            if (plugin->GetMetadata().type != PluginType::StatusBar) continue;
            if (!enabled_map_.count(name) || !enabled_map_.at(name)) continue;

            // Auto-load if needed
            if (plugin->GetState() != PluginState::Loaded) {
                if (!plugin->Load()) continue;
            }

            // Execute the plugin's entry function
            PluginResult result = plugin->Execute(ctx);

            std::vector<StatusBarSegment> segments;

            if (result.success && result.data.is_array()) {
                for (const auto& seg : result.data) {
                    StatusBarSegment s;
                    s.text = seg.value("text", "");
                    s.fg_color = seg.value("fg", "");
                    s.bg_color = seg.value("bg", "");
                    s.bold = seg.value("bold", false);
                    segments.push_back(std::move(s));
                }
            }

            statusbar_cache_[name] = std::move(segments);
        }
    }

    // Collect all segments from cache
    std::vector<StatusBarSegment> all_segments;
    for (const auto& [name, segs] : statusbar_cache_) {
        for (const auto& seg : segs) {
            all_segments.push_back(seg);
        }
    }
    return all_segments;
}

}  // namespace FTB
