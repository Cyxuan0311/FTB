#pragma once

// ─── FTB Plugin System ───────────────────────────────────────────────
//
// A TypeScript/JavaScript plugin system inspired by yazi's plugin architecture.
// Uses QuickJS as the embedded JS runtime for sandboxed execution.
//
// Plugin types:
//   - Functional:  Execute actions (bound to keys or commands)
//   - Previewer:   Custom file preview rendering
//   - Fetcher:     Asynchronous file metadata retrieval
//
// Security model:
//   - Sandboxed JS execution (no direct filesystem/network access)
//   - Permission-based API exposure (fs.read, fs.write, net.fetch, etc.)
//   - Resource limits (execution time, memory)
//   - Plugin isolation (separate contexts)
//
// Plugin directory: ~/.config/ftb/plugins/
//   Each plugin is a directory:
//     my-plugin.ftb/
//     ├── main.ts        (entry point)
//     ├── package.json   (metadata & permissions)
//     └── README.md
// ─────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

namespace FTB {

// ─── Plugin Types ────────────────────────────────────────────────────

enum class PluginType {
    Functional,     // Execute an action
    Previewer,      // Custom file preview
    Fetcher,        // Async metadata fetcher
    StatusBar,      // Contribute to status bar display
};

enum class PluginState {
    Unloaded,
    Loaded,
    Running,
    Error,
    Disabled,
};

// ─── Plugin Permissions ──────────────────────────────────────────────

struct PluginPermissions {
    bool fs_read = false;       // Read files
    bool fs_write = false;      // Write files
    bool fs_list = false;       // List directory contents
    bool net_fetch = false;     // HTTP requests
    bool env_read = false;      // Read environment variables
    bool clipboard = false;     // Access clipboard
    bool subprocess = false;    // Run subprocesses
    int64_t max_exec_ms = 5000; // Max execution time per call

    static PluginPermissions FromJson(const nlohmann::json& j);
    nlohmann::json ToJson() const;
};

// ─── Plugin Metadata ─────────────────────────────────────────────────

struct PluginMetadata {
    std::string name;           // Plugin name (kebab-case)
    std::string version;        // Semver version
    std::string description;    // Short description
    std::string author;         // Author name
    std::string entry;          // Entry file (default: main.ts)
    PluginType type = PluginType::Functional;
    PluginPermissions permissions;
    std::vector<std::string> keywords;  // For previewer/fetcher matching
    std::string min_ftb_version;        // Minimum FTB version required

    static PluginMetadata FromJson(const nlohmann::json& j);
    nlohmann::json ToJson() const;
};

// ─── Plugin Context ──────────────────────────────────────────────────
// Passed to plugin execution, provides runtime info

struct PluginContext {
    std::string current_path;           // Current working directory
    std::string selected_file;          // Currently selected file name
    std::string selected_file_path;     // Full path of selected file
    bool selected_is_dir = false;       // Is selected item a directory?
    int64_t selected_size = 0;          // File size in bytes
    std::string selected_mime;          // MIME type (if known)
    nlohmann::json args;               // Arguments passed to the plugin
};

// ─── Plugin Result ───────────────────────────────────────────────────

struct PluginResult {
    bool success = false;
    std::string error;
    nlohmann::json data;
    std::string message;                // User-facing message to display
};

// ─── Status Bar Segment ──────────────────────────────────────────────
// Returned by StatusBar-type plugins for rendering in the status bar

struct StatusBarSegment {
    std::string text;           // Display text
    std::string fg_color;       // Foreground color (hex or name)
    std::string bg_color;       // Background color (hex or name)
    bool bold = false;
};

// ─── Plugin Instance ─────────────────────────────────────────────────

class PluginInstance {
public:
    PluginInstance(const std::string& plugin_dir, const PluginMetadata& meta);
    ~PluginInstance();

    // Lifecycle
    bool Load();                         // Load and compile the plugin
    void Unload();                       // Unload and free resources
    PluginResult Execute(const PluginContext& ctx);  // Execute plugin entry
    PluginResult ExecuteFunction(const std::string& fn, const PluginContext& ctx);

    // State
    PluginState GetState() const { return state_; }
    const PluginMetadata& GetMetadata() const { return metadata_; }
    const std::string& GetDirectory() const { return plugin_dir_; }
    std::string GetLastError() const { return last_error_; }

    // Hot reload
    bool Reload();

private:
    std::string plugin_dir_;
    PluginMetadata metadata_;
    PluginState state_ = PluginState::Unloaded;
    std::string last_error_;
    std::string compiled_code_;          // Compiled JS code

    // QuickJS context (opaque pointer to avoid header dependency)
    void* js_runtime_ = nullptr;
    void* js_context_ = nullptr;

    bool CompileTypeScript();            // TS -> JS compilation
    bool CreateSandbox();                // Create QuickJS sandboxed context
    void DestroySandbox();
    bool InjectAPI(const PluginContext& ctx);  // Inject FTB API into sandbox
    PluginResult RunInSandbox(const std::string& code, const std::string& fn_name, const PluginContext& ctx);
};

// ─── Plugin Manager ──────────────────────────────────────────────────

class PluginManager {
public:
    static PluginManager* GetInstance();

    // Initialization
    void Initialize(const std::string& config_dir);
    void Shutdown();

    // Discovery
    void ScanPlugins();                  // Scan plugin directory
    std::vector<std::string> GetAvailablePlugins() const;
    const PluginMetadata* GetPluginMetadata(const std::string& name) const;

    // Lifecycle
    bool LoadPlugin(const std::string& name);
    void UnloadPlugin(const std::string& name);
    bool IsPluginLoaded(const std::string& name) const;

    // Execution
    PluginResult ExecutePlugin(const std::string& name, const PluginContext& ctx);
    PluginResult ExecutePluginFunction(const std::string& name,
                                        const std::string& fn,
                                        const PluginContext& ctx);

    // Previewer matching
    std::string FindPreviewer(const std::string& mime_type,
                               const std::string& file_ext) const;

    // Fetcher matching
    std::string FindFetcher(const std::string& mime_type) const;

    // Configuration
    void EnablePlugin(const std::string& name);
    void DisablePlugin(const std::string& name);
    bool IsPluginEnabled(const std::string& name) const;

    // Reload
    void ReloadPlugin(const std::string& name);
    void ReloadAll();

    // Diagnostics
    std::vector<std::string> GetPluginErrors() const;
    nlohmann::json GetPluginStatus(const std::string& name) const;

    // Status bar: collect segments from all enabled StatusBar-type plugins
    std::vector<StatusBarSegment> GetStatusBarContent(const PluginContext& ctx);

private:
    PluginManager() = default;
    ~PluginManager() = default;

    std::string plugins_dir_;
    std::map<std::string, std::unique_ptr<PluginInstance>> plugins_;
    std::map<std::string, bool> enabled_map_;  // name -> enabled
    mutable std::mutex mutex_;
    bool initialized_ = false;

    // Status bar cache: plugin name -> last segments
    std::map<std::string, std::vector<StatusBarSegment>> statusbar_cache_;
    std::chrono::steady_clock::time_point statusbar_last_refresh_;
    static constexpr int STATUSBAR_REFRESH_INTERVAL_MS = 2000;

    std::string ResolvePluginPath(const std::string& name) const;
    bool ValidatePlugin(const std::string& dir) const;
};

}  // namespace FTB
