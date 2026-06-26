#include "../../include/dialog/SSHDialog.hpp"
#include "../../include/remote/SSHConnection.hpp"
#include "../../include/browser/FileManager.hpp"
#include "../../include/config/ConfigManager.hpp"
#include <memory>
#include <ftxui/component/component.hpp>

namespace FTB::UI {

using namespace ftxui;

static std::unique_ptr<Connection::SSHConnection> g_ssh;
static const char* SSH_DATA_DIR = "/home";

Connection::SSHConnection* GetSSHConnection() {
    return g_ssh.get();
}

void disconnectSSH(MainState& state) {
    if (g_ssh) g_ssh->disconnect();
    g_ssh.reset();
    FileManager::setSSHConnection(nullptr);
    state.ssh_connected = false;
    if (!state.ssh_localPath.empty()) {
        state.currentPath = state.ssh_localPath;
    }
    state.active_panel = ActivePanel::None;
    state.ssh_label.clear();
    state.panel_input.clear();
    state.panel_message.clear();
    auto contents = FileManager::getDirectoryContents(state.currentPath);
    state.allContents = contents;
    state.filteredContents = contents;
    state.selected = 0;
}

static Element RenderField(const std::string& label, const std::string& value,
                           int label_w, bool active, bool masked) {
    std::string display;
    if (masked) {
        display = std::string(value.size(), '*');
    } else {
        display = value;
    }
    std::string cursor = active ? "\u258f" : " ";
    auto el = hbox({
        text(label) | color(TC("syn_keyword")) | size(WIDTH, EQUAL, label_w),
        text(display + cursor) | flex,
    });
    if (active) {
        el = el | bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
    } else {
        el = el | color(TC("main_fg"));
    }
    return el;
}

static Element RenderStatusTab(MainState& state) {
    Elements rows;
    rows.push_back(text(""));

    rows.push_back(hbox({
        text(" "),
        text("\u2713 Connected") | color(TC("syn_string")) | bold,
    }));
    rows.push_back(text(""));
    rows.push_back(hbox({ text("   Host:      ") | color(TC("dim")), text(state.ssh_label) | color(TC("main_fg")) | bold }));
    rows.push_back(hbox({ text("   Path:      ") | color(TC("dim")), text(state.ssh_remotePath) | color(TC("main_fg")) }));
    rows.push_back(text(""));
    rows.push_back(text("   Press Delete to disconnect") | color(TC("dim")) | dim);

    return vbox(std::move(rows));
}

static Element RenderFormTab(MainState& state) {
    int lw = 14;
    Elements rows;
    rows.push_back(text(""));

    rows.push_back(RenderField("   Host:       ", state.ssh_host,       lw, state.ssh_field == 0, false));
    rows.push_back(RenderField("   Port:       ", state.ssh_port_str,   lw, state.ssh_field == 1, false));
    rows.push_back(RenderField("   Username:   ", state.ssh_user,       lw, state.ssh_field == 2, false));
    rows.push_back(RenderField("   Password:   ", state.ssh_pass,       lw, state.ssh_field == 3, true));
    rows.push_back(RenderField("   Remote Dir: ", state.ssh_remote_dir, lw, state.ssh_field == 4, false));

    if (!state.panel_message.empty()) {
        rows.push_back(text("   " + state.panel_message) | color(TC("error")));
    }

    return vbox(std::move(rows));
}

static Element RenderRecordsTabContent(MainState& state) {
    Elements rows;
    rows.push_back(text(""));

    if (state.ssh_records.empty()) {
        rows.push_back(text("   (no saved connections)") | color(TC("dim")) | dim);
    } else {
        for (int i = 0; i < static_cast<int>(state.ssh_records.size()); ++i) {
            const auto& rec = state.ssh_records[i];
            bool sel = (i == state.ssh_record_selected);
            std::string line = rec.username + "@" + rec.host + ":" + std::to_string(rec.port)
                             + "  [" + rec.remote_directory + "]";
            auto el = text("   " + line);
            if (sel) {
                el = el | bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold;
            } else {
                el = el | color(TC("main_fg"));
            }
            rows.push_back(el);
        }
    }

    return vbox(std::move(rows));
}

Element RenderSSHPanel(MainState& state, int tw, int th) {
    int pw = std::min(60, tw - 4);
    int ph = std::min(22, th - 4);

    Elements tab_bar;
    tab_bar.push_back(text(" "));
    tab_bar.push_back(text(" Connection ") | (state.ssh_tab == 0
        ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
        : color(TC("dim"))));
    tab_bar.push_back(text(" Records ") | (state.ssh_tab == 1
        ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
        : color(TC("dim"))));
    tab_bar.push_back(text(" "));

    Elements content;
    content.push_back(text(""));

    if (state.ssh_tab == 0) {
        if (state.ssh_connected) {
            content.push_back(RenderStatusTab(state));
        } else {
            content.push_back(text(" SSH Connection") | color(TC("title")) | bold);
            content.push_back(RenderFormTab(state));
        }
    } else {
        content.push_back(text(" Connection History") | color(TC("title")) | bold);
        content.push_back(RenderRecordsTabContent(state));
    }

    content.push_back(text(""));
    content.push_back(text(" Tab=Switch  Esc=Close") | color(TC("dim")) | dim);

    return vbox({
        hbox(tab_bar),
        separator() | color(TC("main_border")),
        vbox(content) | flex,
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
       size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleSSHEvent(MainState& state, const Event& event) {
    if (event == Event::Tab) {
        state.ssh_tab = (state.ssh_tab + 1) % 2;
        return true;
    }

    if (state.ssh_tab == 1) {
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            if (state.ssh_record_selected > 0) state.ssh_record_selected--;
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            if (!state.ssh_records.empty() &&
                state.ssh_record_selected < static_cast<int>(state.ssh_records.size()) - 1)
                state.ssh_record_selected++;
            return true;
        }
        if (event == Event::Return && !state.ssh_records.empty()) {
            const auto& rec = state.ssh_records[state.ssh_record_selected];
            state.ssh_host = rec.host;
            state.ssh_port_str = std::to_string(rec.port);
            state.ssh_user = rec.username;
            state.ssh_remote_dir = rec.remote_directory;
            state.ssh_pass.clear();
            state.ssh_tab = 0;
            state.ssh_field = 4;
            return true;
        }
        if (event == Event::Delete || event == Event::Character('d')) {
            if (!state.ssh_records.empty()) {
                auto& cfg = *FTB::ConfigManager::GetInstance();
                cfg.RemoveSSHRecord(state.ssh_record_selected);
                state.ssh_records = cfg.GetSSHRecords();
                if (state.ssh_record_selected >= static_cast<int>(state.ssh_records.size()))
                    state.ssh_record_selected = std::max(0, static_cast<int>(state.ssh_records.size()) - 1);
            }
            return true;
        }
        return true;
    }

    // Connection tab
    if (state.ssh_connected) {
        if (event == Event::Delete) {
            disconnectSSH(state);
            return true;
        }
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        state.ssh_field = (state.ssh_field > 0) ? state.ssh_field - 1 : 4;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        state.ssh_field = (state.ssh_field + 1) % 5;
        return true;
    }

    if (event == Event::Return) {
        if (state.ssh_host.empty() || state.ssh_user.empty()) {
            state.panel_message = "Host and Username required";
            return true;
        }

        int port = 22;
        try { port = std::stoi(state.ssh_port_str); } catch (...) { port = 22; }
        if (state.ssh_remote_dir.empty()) state.ssh_remote_dir = SSH_DATA_DIR;

        state.panel_message.clear();

        Connection::SSHConnectionParams params;
        params.hostname = state.ssh_host;
        params.port = port;
        params.username = state.ssh_user;
        params.password = state.ssh_pass;
        params.remote_directory = state.ssh_remote_dir;
        params.use_key_auth = false;

        if (!g_ssh) g_ssh = std::make_unique<Connection::SSHConnection>();
        if (g_ssh->connect(params)) {
            state.ssh_connected = true;
            state.ssh_label = state.ssh_user + "@" + state.ssh_host + ":" + std::to_string(port);
            state.ssh_remotePath = state.ssh_remote_dir;
            state.ssh_localPath = state.currentPath;
            state.currentPath = state.ssh_remote_dir;
            state.ssh_field = 0;

            FileManager::setSSHConnection(g_ssh.get());

            auto entries = FileManager::getDirectoryEntries(state.currentPath);
            std::vector<std::string> names;
            names.reserve(entries.size());
            for (const auto& e : entries) names.push_back(e.name);
            state.allContents = names;
            state.filteredContents = names;
            state.selected = 0;

            FTB::SSHRecord rec;
            rec.host = state.ssh_host;
            rec.port = port;
            rec.username = state.ssh_user;
            rec.remote_directory = state.ssh_remote_dir;
            auto& cfg = *FTB::ConfigManager::GetInstance();
            cfg.AddSSHRecord(rec);
            state.ssh_records = cfg.GetSSHRecords();

            state.panel_message = "Connected!";
        } else {
            state.panel_message = "Connection failed: " + g_ssh->getLastError();
        }
        return true;
    }

    auto editField = [&](std::string& field) {
        if (event == Event::Backspace) {
            if (!field.empty()) field.pop_back();
            return true;
        }
        if (event.is_character()) {
            field += event.character();
            return true;
        }
        return false;
    };

    switch (state.ssh_field) {
    case 0: return editField(state.ssh_host);
    case 1: return editField(state.ssh_port_str);
    case 2: return editField(state.ssh_user);
    case 3: return editField(state.ssh_pass);
    case 4: return editField(state.ssh_remote_dir);
    default: return true;
    }
}

}  // namespace FTB::UI
