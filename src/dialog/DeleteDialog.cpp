#include "../../include/dialog/DeleteDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include "../../include/browser/TaskSystem.hpp"
#include "../../include/utils/StatusMessage.hpp"
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderDeleteConfirmPanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(50, tw - 4);

    Elements els;

    els.push_back(text(" Delete") | color(TC("title")) | bold);
    els.push_back(separator() | color(TC("main_border")));
    els.push_back(text(""));

    if (!state.batch_selected.empty()) {
        els.push_back(hbox({
            text(" "),
            text("Delete " + std::to_string(state.batch_selected.size()) + " items?") | color(TC("error")),
        }));
        els.push_back(text(""));

        int count = 0;
        for (int idx : state.batch_selected) {
            if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                if (count >= 5) {
                    els.push_back(hbox({
                        text(" "),
                        text("\u2514 ... and " + std::to_string(state.batch_selected.size() - 5) + " more") | color(TC("dim")) | dim,
                    }));
                    break;
                }
                els.push_back(hbox({
                    text(" "),
                    text("\u251c " + state.filteredContents[idx]) | color(TC("main_fg")),
                }));
                count++;
            }
        }
    } else {
        std::string item_name;
        bool is_dir = false;
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            item_name = state.filteredContents[state.selected];
            fs::path item_path = fs::path(state.currentPath) / item_name;
            is_dir = fs::is_directory(item_path);
        }

        std::string type_label = is_dir ? "directory" : "file";
        els.push_back(hbox({
            text(" "),
            text("Delete " + type_label + "?") | color(TC("error")),
        }));
        els.push_back(text(""));
        els.push_back(hbox({
            text(" "),
            text("\u2192 ") | color(TC("indicator")),
            text(item_name) | color(TC("main_fg")) | bold,
        }));
    }

    els.push_back(text(""));
    els.push_back(!state.panel_message.empty()
        ? text(" " + state.panel_message) | color(TC("error"))
        : text(""));
    els.push_back(!state.panel_message.empty() ? text("") : text(""));
    els.push_back(hbox({
        text(" "),
        text("[Enter]") | color(TC("dim")) | dim,
        text(" Confirm") | color(TC("dim")) | dim,
        text("    "),
        text("[Esc]") | color(TC("dim")) | dim,
        text(" Cancel") | color(TC("dim")) | dim,
        filler(),
    }));

    return vbox(std::move(els)) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleDeleteConfirmEvent(MainState& state, const Event& event) {
    if (event == Event::Return) {
        auto paths = std::make_shared<std::vector<fs::path>>();
        if (!state.batch_selected.empty()) {
            for (int idx : state.batch_selected) {
                if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                    paths->push_back(fs::path(state.currentPath) / state.filteredContents[idx]);
                }
            }
            state.batch_selected.clear();
        } else if (state.selected >= 0 &&
                   state.selected < static_cast<int>(state.filteredContents.size())) {
            paths->push_back(fs::path(state.currentPath) / state.filteredContents[state.selected]);
        }

        if (paths->empty()) {
            state.active_panel = ActivePanel::None;
            state.panel_message.clear();
            return true;
        }

        std::string parent = paths->at(0).parent_path().string();
        int64_t total = static_cast<int64_t>(paths->size());
        StatusMessage::Show("Deleting " + std::to_string(total) + " item(s)... (t: Tasks)");

        TaskRequest req;
        req.title = "Delete " + std::to_string(total) + " item(s)";
        req.type = TaskType::Delete;
        req.priority = Priority::High;
        req.work = [paths, parent](TaskContext& ctx) -> bool {
            int item_count = 0;
            for (const auto& p : *paths) {
                std::error_code ec;
                if (fs::is_directory(p)) {
                    ++item_count;
                    for (auto it = fs::recursive_directory_iterator(
                             p, fs::directory_options::skip_permission_denied, ec);
                         it != fs::recursive_directory_iterator(); it.increment(ec)) {
                        ++item_count;
                    }
                } else {
                    ++item_count;
                }
            }
            ctx.progress.total_files = item_count;

            bool ok = true;
            std::error_code ec;
            for (const auto& p : *paths) {
                if (ctx.cancel.load()) return false;
                while (ctx.pause.load()) {
                    if (ctx.cancel.load()) return false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                ctx.progress.current_file = p.filename().string();

                if (fs::is_directory(p)) {
                    std::vector<fs::path> files;
                    std::vector<fs::path> subdirs;
                    for (auto it = fs::recursive_directory_iterator(
                             p, fs::directory_options::skip_permission_denied, ec);
                         it != fs::recursive_directory_iterator(); it.increment(ec)) {
                        if (ctx.cancel.load()) return false;
                        if (fs::is_regular_file(it->path()) || fs::is_symlink(it->path()))
                            files.push_back(it->path());
                        else if (fs::is_directory(it->path()))
                            subdirs.push_back(it->path());
                    }
                    for (const auto& f : files) {
                        if (ctx.cancel.load()) return false;
                        fs::remove(f, ec);
                        ++ctx.progress.files_processed;
                    }
                    for (auto it = subdirs.rbegin(); it != subdirs.rend(); ++it) {
                        if (ctx.cancel.load()) return false;
                        fs::remove(*it, ec);
                        ++ctx.progress.files_processed;
                    }
                    fs::remove(p, ec);
                    ++ctx.progress.files_processed;
                    if (ec) ok = false;
                } else if (fs::is_regular_file(p) || fs::is_symlink(p)) {
                    fs::remove(p, ec);
                    ++ctx.progress.files_processed;
                    if (ec) ok = false;
                }
            }

            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_dir_cache->erase(parent);
                FileManager::lru_entry_cache->erase(parent);
            }
            return ok;
        };
        req.callback = [&state](const std::string&, TaskState ts) {
            if (ts == TaskState::Completed || ts == TaskState::Failed ||
                ts == TaskState::Cancelled) {
                state.refresh_pending.store(true);
                if (state.screen)
                    state.screen->Post(ftxui::Event::Custom);
            }
        };

        TaskSystem::getInstance().submit(std::move(req));

        state.active_panel = ActivePanel::None;
        state.panel_message.clear();
        return true;
    }

    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_message.clear();
        return true;
    }

    return true;
}

}  // namespace FTB::UI
