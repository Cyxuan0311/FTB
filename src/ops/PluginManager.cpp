#include "../../include/ops/PluginManager.hpp"
#include "../../include/utils/PerfLogger.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <regex>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <fcntl.h>

namespace fs = std::filesystem;

namespace FTB {

// ─── Platform Detection ───────────────────────────────────────────────

std::string PluginManager::DetectPlatform() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    // Check for WSL by reading /proc/version
    std::ifstream ifs("/proc/version");
    if (ifs) {
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        if (content.find("Microsoft") != std::string::npos ||
            content.find("microsoft") != std::string::npos ||
            content.find("WSL") != std::string::npos) {
            return "wsl";
        }
    }
    return "linux";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#elif defined(__NetBSD__)
    return "netbsd";
#else
    return "unknown";
#endif
}

std::string PluginManager::DetectDistroId() {
#if defined(__linux__)
    std::ifstream ifs("/etc/os-release");
    if (!ifs) {
        // Fallback: /usr/lib/os-release
        ifs.open("/usr/lib/os-release");
    }
    if (ifs) {
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.rfind("ID=", 0) == 0) {
                std::string id = line.substr(3);
                // Remove surrounding quotes if present
                if (id.size() >= 2 && id.front() == '"' && id.back() == '"') {
                    id = id.substr(1, id.size() - 2);
                }
                return id;
            }
        }
    }
#endif
    return "";
}

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
    if (j.contains("python_exec")) p.python_exec = j["python_exec"].get<bool>();
    if (j.contains("max_exec_ms")) p.max_exec_ms = j["max_exec_ms"].get<int64_t>();
    return p;
}

nlohmann::json PluginPermissions::ToJson() const {
    return nlohmann::json{
        {"fs_read", fs_read}, {"fs_write", fs_write}, {"fs_list", fs_list},
        {"net_fetch", net_fetch}, {"env_read", env_read}, {"clipboard", clipboard},
        {"subprocess", subprocess}, {"python_exec", python_exec}, {"max_exec_ms", max_exec_ms}
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

    if (j.contains("commands") && j["commands"].is_array()) {
        for (const auto& c : j["commands"])
            m.commands.push_back(c.get<std::string>());
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
    if (!commands.empty()) j["commands"] = commands;
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
    if (state_ == PluginState::Loaded) {
        return true;
    }

    PERF_LOG("PluginLoad", "Loading: " + metadata_.name + " from " + plugin_dir_);

    fs::path entry_path = fs::path(plugin_dir_) / metadata_.entry;
    if (!fs::exists(entry_path)) {
        entry_path = fs::path(plugin_dir_) /
                     fs::path(metadata_.entry).replace_extension(".js");
        PERF_LOG("PluginLoad", "  entry not found, trying .js fallback");
    }

    if (!fs::exists(entry_path)) {
        last_error_ = "Entry file not found: " + metadata_.entry;
        state_ = PluginState::Error;
        PERF_LOG("PluginLoad", "  ERROR: " + last_error_);
        return false;
    }

    std::ifstream ifs(entry_path);
    if (!ifs.is_open()) {
        last_error_ = "Cannot open entry file";
        state_ = PluginState::Error;
        PERF_LOG("PluginLoad", "  ERROR: " + last_error_);
        return false;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    compiled_code_ = oss.str();
    PERF_LOG("PluginLoad", "  read " + std::to_string(compiled_code_.size()) + " bytes from " + entry_path.string());

    if (entry_path.extension() == ".ts") {
        PERF_LOG("PluginLoad", "  transpiling TypeScript...");
        if (!CompileTypeScript()) {
            state_ = PluginState::Error;
            PERF_LOG("PluginLoad", "  ERROR: TypeScript compilation failed");
            return false;
        }
        PERF_LOG("PluginLoad", "  transpilation OK, " + std::to_string(compiled_code_.size()) + " bytes");
    }

    if (!CreateSandbox()) {
        state_ = PluginState::Error;
        PERF_LOG("PluginLoad", "  ERROR: CreateSandbox failed");
        return false;
    }

    state_ = PluginState::Loaded;
    PERF_LOG("PluginLoad", "  loaded OK");
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

    // Set plugin_dir for ftb.python.call() resolution
    PluginContext ctx_with_dir = ctx;
    ctx_with_dir.plugin_dir = plugin_dir_;

    PluginResult result = RunInSandbox(compiled_code_, fn, ctx_with_dir);

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
    // Phase 1: Remove `interface X { ... }` blocks with a brace-depth counter
    {
        std::string result;
        std::istringstream stream(compiled_code_);
        std::string line;
        int interface_depth = 0;

        while (std::getline(stream, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));

            if (interface_depth == 0 && trimmed.find("interface ") == 0) {
                interface_depth = 0;
                for (char c : line) {
                    if (c == '{') interface_depth++;
                    if (c == '}') interface_depth--;
                }
                continue;
            }

            if (interface_depth > 0) {
                for (char c : line) {
                    if (c == '{') interface_depth++;
                    if (c == '}') interface_depth--;
                }
                if (interface_depth > 0) continue;
                continue;
            }

            result += line + "\n";
        }
        compiled_code_ = result;
    }

    // Phase 2: Apply regex replacements for other TypeScript constructs
    static const std::vector<std::pair<std::string, std::string>> replacements = {
        {"type\\s+\\w+\\s*(<[^>]*>)?\\s*=\\s*[^;]+;", ""},
        {"import\\s+type\\s+\\{[^}]*\\}\\s+from\\s+['\"][^'\"]*['\"];?", ""},
        {"\\s+as\\s+[A-Z]\\w*(<[^>]*>)?", ""},
        {"\\)\\s*:\\s*[A-Za-z_]\\w*(?:\\[\\])?\\s*=>", ") =>"},
        {"\\)\\s*:\\s*[A-Za-z_]\\w*(?:\\[\\])?\\s*\\{", ") {"},
        {"\\)\\s*:\\s*(?:[A-Za-z_]\\w*(?:\\[\\])?(?:\\s*\\|\\s*[A-Za-z_]\\w*(?:\\[\\])?)*)\\s*=>", ") =>"},
        {"\\)\\s*:\\s*(?:[A-Za-z_]\\w*(?:\\[\\])?(?:\\s*\\|\\s*[A-Za-z_]\\w*(?:\\[\\])?)*)\\s*\\{", ") {"},
        {"\\((\\w+):\\s*[A-Za-z_]\\w*(?:\\[\\])?\\s*\\)", "($1)"},
        {"\\(\\s*(\\w+):\\s*[A-Za-z_]\\w*(?:\\[\\])?,\\s*", "($1, "},
        {",\\s*(\\w+):\\s*(?:[A-Z]\\w*|string|number|boolean|bigint|symbol)(?:\\[\\])?(?!\\s*[\\.\\(])(?:\\s*=\\s*[^,)]+)?", ",$1"},
        {"(let|var)\\s+(\\w+):\\s*[A-Za-z_]\\w*(?:<[^>]*>)?(?:\\[\\])?\\s*;", "$1 $2;"},
        {"(const|let|var)\\s+(\\w+):\\s*[A-Za-z_]\\w*(?:<[^>]*>)?(?:\\[\\])?\\s*=", "$1 $2 ="},
    };

    for (const auto& [pattern, replacement] : replacements) {
        try {
            compiled_code_ = std::regex_replace(compiled_code_, std::regex(pattern), replacement);
        } catch (const std::regex_error&) {
        }
    }

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
    PluginResult result;

    // Build the execution wrapper with QuickJS built-in modules for subprocess mode
    std::string wrapped_code = R"(
(function() {
    'use strict';
    // ---- FTB Plugin Runtime (subprocess mode with QuickJS-ng) ----
    var __ftb_tmpdir = '/tmp/ftb_plugin_' + os.getpid();
    var __ftb_plugin_dir = )" + nlohmann::json(plugin_dir_).dump() + R"(;

    function __ftb_result(data) {
        std.out.puts(data);
    }

    function __ftb_call(method, args) {
        switch (method) {
            case 'exec': {
                var cmd = args[0];
                var cmdArgs = args[1] || [];
                var fullCmd = cmd;
                for (var i = 0; i < cmdArgs.length; i++) {
                    var a = String(cmdArgs[i]);
                    fullCmd += " '" + a.replace(/'/g, "'\\''") + "'";
                }
                var outfile = __ftb_tmpdir + '_exec.out';
                os.exec(['/bin/sh', '-c', fullCmd + ' > ' + outfile + ' 2>&1']);
                var output = '';
                try {
                    var loaded = std.loadFile(outfile);
                    if (loaded !== null) output = loaded;
                } catch(e) {}
                try { os.remove(outfile); } catch(e) {}
                return output;
            }
            case 'fs.readFile': {
                return std.loadFile(args[0]);
            }
            case 'fs.writeFile': {
                var wpath = args[0].replace(/'/g, "'\\''");
                var wcontent = args[1].replace(/'/g, "'\\''");
                os.exec(['/bin/sh', '-c', "printf '%s' '" + wcontent + "' > '" + wpath + "'"]);
                return '';
            }
            case 'fs.listDir': {
                var outfile = __ftb_tmpdir + '_ls.out';
                os.exec(['/bin/sh', '-c', 'ls -1 "' + args[0].replace(/"/g, '\\"') + '" > ' + outfile + ' 2>/dev/null']);
                var output = '';
                try {
                    var loaded = std.loadFile(outfile);
                    if (loaded !== null) output = loaded;
                } catch(e) {}
                try { os.remove(outfile); } catch(e) {}
                var entries = [];
                var lines = output.split('\n');
                for (var i = 0; i < lines.length; i++) {
                    var trimmed = lines[i].trim();
                    if (trimmed.length > 0) entries.push(trimmed);
                }
                return entries;
            }
            case 'env.get': {
                return os.getenv(args[0]);
            }
            case 'log': {
                std.err.puts('[ftb:plugin] ' + args[0] + '\n');
                return '';
            }
            case 'clipboard.read':
            case 'clipboard.write':
            case 'ui.message':
            case 'ui.confirm':
            case 'statusBar.set': {
                return '';
            }
            case 'python.call': {
                var pyFile = String(args[0] || '');
                var pyFunc = String(args[1] || '');
                var pyArgs = args[2] || {};
                var plugDir = __ftb_plugin_dir;
                var outfile = __ftb_tmpdir + '_pyout.json';
                var argsfile = __ftb_tmpdir + '_pyargs.json';
                var wrapfile = __ftb_tmpdir + '_pywrap.py';

                // Write args as JSON to temp file
                os.exec(['/bin/sh', '-c',
                    "printf '%s' '" + JSON.stringify(pyArgs).replace(/'/g,"'\\''") + "' > '" + argsfile + "'"]);

                // Build Python wrapper script
                var modName = pyFile.replace(/\.py$/g,'').replace(/\//g,'.');
                var pyScript = "import sys,json\\n";
                pyScript += "sys.path.insert(0,'" + plugDir.replace(/'/g,"'\\''") + "')\\n";
                pyScript += "try:\\n";
                pyScript += "    mod = __import__('" + modName.replace(/'/g,"'\\''") + "')\\n";
                pyScript += "    with open('" + argsfile.replace(/'/g,"'\\''") + "') as f: a = json.load(f)\\n";
                pyScript += "    r = mod." + pyFunc + "(**a)\\n";
                pyScript += "    print(json.dumps({'success':True,'data':r}))\\n";
                pyScript += "except Exception as e:\\n";
                pyScript += "    print(json.dumps({'success':False,'error':str(e)}))\\n";

                // Write wrapper file
                os.exec(['/bin/sh', '-c',
                    "printf '%b' '" + pyScript.replace(/'/g,"'\\''") + "' > '" + wrapfile + "'"]);

                // Execute Python
                os.exec(['/bin/sh', '-c', "python3 -u '" + wrapfile + "' > '" + outfile + "' 2>&1"]);

                // Read result
                try {
                    var output = std.loadFile(outfile);
                    if (output) {
                        output = output.trim();
                        if (output.length > 0) return JSON.parse(output);
                    }
                } catch(e) {}
                finally {
                    try { os.remove(argsfile); } catch(e) {}
                    try { os.remove(wrapfile); } catch(e) {}
                    try { os.remove(outfile); } catch(e) {}
                }
                return {success: false, error: 'Python call failed'};
            }
            default: {
                throw new Error('Unknown API method: ' + method);
            }
        }
    }

    // ---- FTB Plugin API ----
    var ftb = {
        ctx: )" + nlohmann::json({
            {"current_path", ctx.current_path},
            {"selected_file", ctx.selected_file},
            {"selected_file_path", ctx.selected_file_path},
            {"selected_is_dir", ctx.selected_is_dir},
            {"selected_size", ctx.selected_size},
            {"selected_mime", ctx.selected_mime},
            {"args", ctx.args},
            {"platform", ctx.platform},
            {"distro_id", ctx.distro_id},
            {"plugin_dir", ctx.plugin_dir}
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
        python: {
            call: function(file, func, args) { return __ftb_call('python.call', [file, func, args || {}]); },
        },
        log: function(msg) { __ftb_call('log', [msg]); },
        exec: function(cmd, args) { return __ftb_call('exec', [cmd, args || []]); },
    };

    // ---- Plugin Code ----
)" + code + R"(

    // ---- Execute ----
    if (typeof )" + fn_name + R"( === 'function') {
        try {
            var result = )" + fn_name + R"((ftb);
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
    PERF_LOG("PluginExec", "qjs path: " + qjs_path + " (exists=" + (fs::exists(qjs_path) ? "yes" : "no") + ")");

    if (fs::exists(qjs_path)) {
        std::string tmp_path = "/tmp/ftb_plugin_" + metadata_.name + ".js";
        {
            std::ofstream ofs(tmp_path);
            ofs << wrapped_code;
        }
        PERF_LOG("PluginExec", "wrote temp JS: " + tmp_path + " (" + std::to_string(wrapped_code.size()) + " bytes)");

        std::string err_path = tmp_path + ".err";
        int timeout_secs = std::max<int>(metadata_.permissions.max_exec_ms / 1000, 1);
        std::string cmd = "timeout " + std::to_string(timeout_secs) +
                         " " + qjs_path + " --std " + tmp_path + " 2>" + err_path;
        PERF_LOG("PluginExec", "cmd: " + cmd);

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.error = "Failed to start JS runtime";
            PERF_LOG("PluginExec", "ERROR: popen failed");
            fs::remove(tmp_path);
            fs::remove(err_path);
            return result;
        }

        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        int exit_code = pclose(pipe);

        std::string err_output;
        if (exit_code != 0) {
            std::ifstream err_ifs(err_path);
            if (err_ifs) {
                std::ostringstream err_oss;
                err_oss << err_ifs.rdbuf();
                err_output = err_oss.str();
            }
            PERF_LOG("PluginExec", "exit_code=" + std::to_string(exit_code) + " stderr=" + err_output);
        }

        fs::remove(tmp_path);
        fs::remove(err_path);

        if (exit_code != 0) {
            result.success = false;
            result.error = "Plugin exited with code " + std::to_string(exit_code) + ": " + err_output;
            PERF_LOG("PluginExec", "FAILED: " + result.error);
            return result;
        }

        PERF_LOG("PluginExec", "stdout (" + std::to_string(output.size()) + " bytes): " + output.substr(0, 200));

        try {
            auto j = nlohmann::json::parse(output);
            result.success = j.value("success", false);
            if (j.contains("data")) result.data = j["data"];
            if (j.contains("error")) result.error = j["error"].get<std::string>();
            if (j.contains("message")) result.message = j.value("message", "");
            PERF_LOG("PluginExec", "parse OK, success=" + std::string(result.success ? "true" : "false"));
        } catch (const nlohmann::json::parse_error& e) {
            result.success = true;
            result.message = output;
            PERF_LOG("PluginExec", "JSON parse error, treating as raw message: " + std::string(e.what()));
        }
    } else {
        result.success = false;
        result.error = "No JavaScript runtime found. Install QuickJS (qjs) to enable plugins.";
        PERF_LOG("PluginExec", "ERROR: " + result.error);
    }

    return result;
}

// ─── PluginManager ───────────────────────────────────────────────────

PluginManager* PluginManager::GetInstance() {
    static PluginManager instance;
    return &instance;
}

void PluginManager::Initialize(const std::string& config_dir) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    if (initialized_) {
        return;
    }

    config_dir_ = config_dir;
    plugins_dir_ = config_dir + "/plugins";

    PERF_LOG("Plugin", "Init from: " + plugins_dir_);

    if (!fs::exists(plugins_dir_)) {
        fs::create_directories(plugins_dir_);
        PERF_LOG("Plugin", "Created plugins dir");
    }

    platform_ = DetectPlatform();
    distro_id_ = DetectDistroId();
    PERF_LOG("Plugin", "Platform: " + platform_ + ", Distro: " + distro_id_);

    ScanPlugins();

    LoadEnabledState();

    initialized_ = true;

    PERF_LOG("Plugin", "Loaded " + std::to_string(plugins_.size()) + " plugins");
    for (const auto& [name, inst] : plugins_) {
        bool en = enabled_map_.count(name) && enabled_map_.at(name);
        PERF_LOG("Plugin", "  " + name + " (type=" + std::to_string(static_cast<int>(inst->GetMetadata().type)) + " enabled=" + (en ? "yes" : "no") + ")");
    }
}

void PluginManager::Shutdown() {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    for (auto& [name, plugin] : plugins_) {
        if (plugin->GetState() == PluginState::Loaded) {
            plugin->Unload();
        }
    }
    plugins_.clear();
    initialized_ = false;
}

void PluginManager::ScanPlugins() {
    if (!fs::exists(plugins_dir_)) {
        PERF_LOG("Plugin", "ScanPlugins: dir not found: " + plugins_dir_);
        return;
    }

    int found = 0, skipped = 0;
    for (const auto& entry : fs::directory_iterator(plugins_dir_)) {
        if (!entry.is_directory()) continue;

        std::string dir_name = entry.path().filename().string();

        if (dir_name.size() > 4 && dir_name.substr(dir_name.size() - 4) == ".ftb") {
            std::string plugin_name = dir_name.substr(0, dir_name.size() - 4);

            if (ValidatePlugin(entry.path().string())) {
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

                        if (enabled_map_.find(plugin_name) == enabled_map_.end()) {
                            enabled_map_[plugin_name] = true;
                        }
                        found++;
                        PERF_LOG("Plugin", "  Found plugin: " + plugin_name + " (" + meta.entry + ")");
                    } catch (const std::exception& e) {
                        PERF_LOG("Plugin", "  ERROR loading " + plugin_name + ": " + e.what());
                        skipped++;
                    }
                }
            } else {
                PERF_LOG("Plugin", "  Validation failed: " + plugin_name);
                skipped++;
            }
        }
    }
    PERF_LOG("Plugin", "ScanPlugins: found=" + std::to_string(found) + ", skipped=" + std::to_string(skipped));
}

std::vector<std::string> PluginManager::GetAvailablePlugins() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    std::vector<std::string> names;
    for (const auto& [name, plugin] : plugins_) {
        names.push_back(name);
    }
    return names;
}

const PluginMetadata* PluginManager::GetPluginMetadata(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return &it->second->GetMetadata();
    }
    return nullptr;
}

bool PluginManager::LoadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end()) return false;

    if (!enabled_map_[name]) {
        return false;  // Plugin is disabled
    }

    return it->second->Load();
}

void PluginManager::UnloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        it->second->Unload();
    }
}

bool PluginManager::IsPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second->GetState() == PluginState::Loaded;
    }
    return false;
}

PluginResult PluginManager::ExecutePlugin(const std::string& name, const PluginContext& ctx) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

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
    PluginInstance* instance = nullptr;
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);

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

        instance = it->second.get();
    }

    return instance->ExecuteFunction(fn, ctx);
}

std::string PluginManager::FindPreviewer(const std::string& mime_type,
                                           const std::string& file_ext) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

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
    std::lock_guard<std::mutex> lock(plugins_mutex_);

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
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        enabled_map_[name] = true;
    }
    SaveEnabledState();
}

void PluginManager::DisablePlugin(const std::string& name) {
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        enabled_map_[name] = false;

        auto it = plugins_.find(name);
        if (it != plugins_.end()) {
            it->second->Unload();
        }
    }
    SaveEnabledState();
}

bool PluginManager::IsPluginEnabled(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    auto it = enabled_map_.find(name);
    return it != enabled_map_.end() && it->second;
}

void PluginManager::ReloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        it->second->Reload();
    }
}

void PluginManager::ReloadAll() {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    for (auto& [name, plugin] : plugins_) {
        plugin->Reload();
    }
}

std::vector<std::string> PluginManager::GetPluginErrors() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    std::vector<std::string> errors;
    for (const auto& [name, plugin] : plugins_) {
        if (plugin->GetState() == PluginState::Error) {
            errors.push_back(name + ": " + plugin->GetLastError());
        }
    }
    return errors;
}

nlohmann::json PluginManager::GetPluginStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

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

PluginResult PluginManager::ExecutePluginCommand(const std::string& plugin_name,
                                                   const PluginContext& ctx) {
    PluginContext cmd_ctx = ctx;
    cmd_ctx.args = nlohmann::json::object();
    if (!ctx.args.is_null() && ctx.args.is_string()) {
        cmd_ctx.args["command"] = ctx.args.get<std::string>();
    }
    auto result = ExecutePlugin(plugin_name, cmd_ctx);

    // Invalidate previewer definitions cache for this plugin
    if (result.success) {
        std::string prefix = plugin_name + "|";
        for (auto it = previewer_definitions_.begin(); it != previewer_definitions_.end(); ) {
            if (it->first.size() >= prefix.size() &&
                it->first.substr(0, prefix.size()) == prefix) {
                it = previewer_definitions_.erase(it);
            } else {
                ++it;
            }
        }
    }

    return result;
}

std::vector<std::string> PluginManager::CompletePluginCommand(const std::string& input) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);

    auto sp = input.find(' ');
    if (sp == std::string::npos) {
        // Completing plugin name
        std::vector<std::string> matches;
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& [name, _] : plugins_) {
            std::string name_lower = name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            if (name_lower.size() >= lower.size() &&
                name_lower.substr(0, lower.size()) == lower) {
                matches.push_back(name);
            }
        }
        return matches;
    }

    // Completing plugin command after plugin name
    std::string plugin_name = input.substr(0, sp);
    std::string cmd_partial = input.substr(sp + 1);
    std::string cmd_lower = cmd_partial;
    std::transform(cmd_lower.begin(), cmd_lower.end(), cmd_lower.begin(), ::tolower);

    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        // Try case-insensitive match
        for (const auto& [name, plugin] : plugins_) {
            std::string name_lower = name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            if (name_lower == plugin_name) {
                it = plugins_.find(name);
                break;
            }
        }
    }

    if (it == plugins_.end()) return {};

    std::vector<std::string> matches;
    for (const auto& cmd : it->second->GetMetadata().commands) {
        std::string cmd_lc = cmd;
        std::transform(cmd_lc.begin(), cmd_lc.end(), cmd_lc.begin(), ::tolower);
        if (cmd_lc.size() >= cmd_lower.size() &&
            cmd_lc.substr(0, cmd_lower.size()) == cmd_lower) {
            matches.push_back(plugin_name + " " + cmd);
        }
    }
    return matches;
}

// ─── Previewer: Load Definition ───────────────────────────────────────
// Runs the plugin's entry() once (via qjs) to extract the command template.
// The definition is cached for subsequent previews.

PreviewerDefinition PluginManager::LoadPreviewerDefinition(const std::string& name, const PluginContext& ctx) {
    std::string cache_key = name + "|" + fs::path(ctx.selected_file_path).extension().string();
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        auto it = previewer_definitions_.find(cache_key);
        if (it != previewer_definitions_.end() && it->second.loaded) {
            return it->second;
        }
    }

    PluginContext def_ctx = ctx;
    def_ctx.panel_width = std::max(def_ctx.panel_width, 80);

    auto result = ExecutePluginFunction(name, "entry", def_ctx);
    PreviewerDefinition def;

    if (result.success && result.data.is_object()) {
        auto& d = result.data;
        if (d.contains("command") && d["command"].is_string())
            def.command = d["command"].get<std::string>();
        if (d.contains("args") && d["args"].is_array()) {
            for (const auto& a : d["args"])
                def.args.push_back(a.get<std::string>());
        }
        if (d.contains("label") && d["label"].is_string())
            def.label = d["label"].get<std::string>();
        if (d.contains("ansi") && d["ansi"].is_boolean())
            def.ansi_output = d["ansi"].get<bool>();
        if (d.contains("timeout") && d["timeout"].is_number())
            def.timeout_ms = d["timeout"].get<int>();
    }

    if (def.command.empty()) {
        def.loaded = false;
    } else {
        def.loaded = true;
        if (def.label.empty()) def.label = name;
    }

    if (def.loaded) {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        previewer_definitions_[cache_key] = def;
    }

    return def;
}

// ─── Previewer: Cancel ────────────────────────────────────────────────

void PluginManager::CancelPluginPreview() {
    std::lock_guard<std::mutex> lock(preview_mutex_);
    if (preview_entry_.child_pid > 0) {
        ::kill(-preview_entry_.child_pid, SIGTERM);
        ::waitpid(preview_entry_.child_pid, nullptr, WNOHANG);
        preview_entry_.child_pid = 0;
    }
}

// ─── Previewer: Execute ───────────────────────────────────────────────
// Lazy-loads the command definition, expands placeholders, then forks a
// subprocess to run the external command asynchronously.

void PluginManager::ExecutePluginPreview(const std::string& name,
                                           const PluginContext& ctx,
                                           int panel_width) {
    auto def = LoadPreviewerDefinition(name, ctx);
    if (!def.loaded) {
        std::lock_guard<std::mutex> lock(preview_mutex_);
        preview_entry_ = {name, ctx.selected_file_path, "", name, true, true, true, 0};
        return;
    }

    {
        std::lock_guard<std::mutex> lock(preview_mutex_);
        if (preview_entry_.file_path == ctx.selected_file_path
            && preview_entry_.plugin_name == name
            && preview_entry_.loaded) {
            return;
        }
    }

    CancelPluginPreview();

    auto ShellQuote = [](const std::string& val) -> std::string {
        std::string result = "'";
        for (char c : val) {
            if (c == '\'') result += "'\\''";
            else result += c;
        }
        result += '\'';
        return result;
    };

    auto Expand = [&](std::string s) -> std::string {
        auto r = [&](const std::string& from, const std::string& to) -> bool {
            size_t p;
            if ((p = s.find(from)) != std::string::npos) {
                s.replace(p, from.size(), ShellQuote(to));
                return true;
            }
            return false;
        };
        r("{file}", ctx.selected_file_path);
        r("{name}", ctx.selected_file);
        r("{width}", std::to_string(panel_width));
        return s;
    };

    std::string cmd = Expand(def.command);
    for (const auto& a : def.args)
        cmd += " " + Expand(a);

    if (def.timeout_ms > 0) {
        cmd = "timeout " + std::to_string(def.timeout_ms / 1000) + "s " + cmd;
    }

    {
        std::lock_guard<std::mutex> lock(preview_mutex_);
        preview_entry_.plugin_name = name;
        preview_entry_.file_path = ctx.selected_file_path;
        preview_entry_.output.clear();
        preview_entry_.label = def.label;
        preview_entry_.completed = false;
        preview_entry_.failed = false;
        preview_entry_.child_pid = 0;
        preview_entry_.loaded = true;
    }

    std::thread([this, cmd, path = ctx.selected_file_path]() {
        int pipefd[2];
        if (::pipe(pipefd) == -1) return;

        pid_t pid = ::fork();
        if (pid == -1) {
            ::close(pipefd[0]);
            ::close(pipefd[1]);
            return;
        }

        if (pid == 0) {
            ::close(pipefd[0]);
            ::dup2(pipefd[1], STDOUT_FILENO);
            int fd = ::open("/dev/null", O_WRONLY);
            ::dup2(fd, STDERR_FILENO);
            for (int i = 3; i < 1024; i++) ::close(i);
            ::setpgid(0, 0);
            ::execlp("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            ::_exit(1);
        }

        ::close(pipefd[1]);
        {
            std::lock_guard<std::mutex> lock(preview_mutex_);
            preview_entry_.child_pid = pid;
        }

        std::string result;
        char buf[4096];
        ssize_t n;
        while ((n = ::read(pipefd[0], buf, sizeof(buf))) > 0)
            result.append(buf, n);
        ::close(pipefd[0]);

        int status;
        ::waitpid(pid, &status, 0);

        {
            std::lock_guard<std::mutex> lock(preview_mutex_);
            if (preview_entry_.file_path != path) return;
            preview_entry_.output = std::move(result);
            preview_entry_.failed = preview_entry_.output.empty();
            preview_entry_.completed = true;
            preview_entry_.child_pid = 0;
        }

    }).detach();
}

// ─── Previewer: Poll Result ───────────────────────────────────────────

bool PluginManager::PollPluginPreview(const std::string& name,
                                        PluginPreviewResult& out) {
    std::lock_guard<std::mutex> lock(preview_mutex_);
    if (preview_entry_.plugin_name != name || !preview_entry_.loaded) {
        return false;
    }
    out.label = preview_entry_.label;
    out.loaded = true;
    out.completed = preview_entry_.completed;
    out.failed = preview_entry_.failed;
    out.output = preview_entry_.output;
    return true;
}

// ─── Status Bar Content (Async) ─────────────────────────────────────

std::vector<StatusBarSegment> PluginManager::GetStatusBarContent(const PluginContext& ctx) {
    // Store a snapshot of the context for the background thread
    {
        std::lock_guard<std::mutex> lock(ctx_mutex_);
        statusbar_context_snapshot_ = ctx;
    }

    // Return cached segments immediately — never blocks on plugin execution
    std::lock_guard<std::mutex> lock(statusbar_mutex_);
    return statusbar_cached_segments_;
}

void PluginManager::UpdateContextSnapshot(const PluginContext& ctx) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    statusbar_context_snapshot_ = ctx;
}

// ─── Background Refresh ──────────────────────────────────────────────

void PluginManager::StartBackgroundRefresh(std::function<void()> on_updated) {
    if (background_running_.exchange(true)) {
        return;
    }

    on_statusbar_updated_ = std::move(on_updated);
    background_thread_ = std::thread(&PluginManager::BackgroundRefreshWorker, this);
}

void PluginManager::StopBackgroundRefresh() {
    background_running_ = false;
    shutdown_cv_.notify_one();
    if (background_thread_.joinable()) {
        // Wait up to 500ms for the worker to finish its current cycle
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (background_thread_.joinable() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (background_thread_.joinable()) {
            background_thread_.detach();
        } else {
            background_thread_.join();
        }
    }
}

void PluginManager::BackgroundRefreshWorker() {
    while (background_running_) {
        {
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            shutdown_cv_.wait_for(lock,
                std::chrono::milliseconds(STATUSBAR_REFRESH_INTERVAL_MS),
                [this] { return !background_running_; });
        }
        if (!background_running_) break;

        PluginContext ctx;
        {
            std::lock_guard<std::mutex> lock(ctx_mutex_);
            ctx = statusbar_context_snapshot_;
        }
        ctx.platform = platform_;
        ctx.distro_id = distro_id_;

        if (ctx.current_path.empty()) continue;

        auto segments = ExecuteStatusBarPlugins(ctx);

        {
            std::lock_guard<std::mutex> lock(statusbar_mutex_);
            statusbar_cached_segments_ = std::move(segments);
        }

        if (on_statusbar_updated_) {
            on_statusbar_updated_();
        }
    }
}

std::vector<StatusBarSegment> PluginManager::ExecuteStatusBarPlugins(const PluginContext& ctx) {
    std::vector<StatusBarSegment> all_segments;
    std::vector<PluginInstance*> instances;

    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);

        for (auto& [name, plugin] : plugins_) {
            if (plugin->GetMetadata().type != PluginType::StatusBar) continue;
            if (!enabled_map_.count(name) || !enabled_map_.at(name)) continue;

            if (plugin->GetState() != PluginState::Loaded) {
                if (!plugin->Load()) {
                    continue;
                }
            }

            instances.push_back(plugin.get());
        }
    }

    PERF_LOG("StatusBar", "executing " + std::to_string(instances.size()) + " statusbar plugins");

    for (auto* instance : instances) {
        PERF_LOG("StatusBar", "  running: " + instance->GetMetadata().name);
        PluginResult result = instance->Execute(ctx);

        if (result.success && result.data.is_array()) {
            PERF_LOG("StatusBar", "    OK, segments=" + std::to_string(result.data.size()));
            for (const auto& seg : result.data) {
                StatusBarSegment s;
                s.text = seg.value("text", "");
                s.fg_color = seg.value("fg", "");
                s.bg_color = seg.value("bg", "");
                s.bold = seg.value("bold", false);
                s.align = seg.value("align", "left");
                all_segments.push_back(std::move(s));
            }
        } else {
            PERF_LOG("StatusBar", "    FAILED: " + result.error);
        }
    }

    PERF_LOG("StatusBar", "total segments: " + std::to_string(all_segments.size()));
    return all_segments;
}

// ─── Enabled State Persistence ──────────────────────────────────────

void PluginManager::SaveEnabledState() {
    if (config_dir_.empty()) return;

    std::lock_guard<std::mutex> lock(plugins_mutex_);

    nlohmann::json j;
    for (const auto& [name, enabled] : enabled_map_) {
        j[name] = enabled;
    }

    std::string path = config_dir_ + "/plugins_enabled.json";
    try {
        std::ofstream ofs(path);
        ofs << j.dump(2);
    } catch (const std::exception& e) {
    }
}

void PluginManager::LoadEnabledState() {
    if (config_dir_.empty()) return;

    std::string path = config_dir_ + "/plugins_enabled.json";
    if (!fs::exists(path)) return;

    try {
        std::ifstream ifs(path);
        nlohmann::json j;
        ifs >> j;

        for (auto it = j.begin(); it != j.end(); ++it) {
            if (it.value().is_boolean()) {
                enabled_map_[it.key()] = it.value().get<bool>();
            }
        }
    } catch (const std::exception& e) {
    }
}

}  // namespace FTB
