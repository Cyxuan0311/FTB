#include "../../include/dialog/FolderDetailsDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

static std::string formatSize(uintmax_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    if (unit == 0) return std::to_string(bytes) + " B";
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return os.str();
}

static std::string formatTime(const fs::file_time_type& ft) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ft - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm* tm = std::localtime(&tt);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

static Element makeSection(const std::string& title, int width) {
    std::string head = "\u2500\u2500 ";
    int remaining = width - static_cast<int>(head.size() + title.size() + 1);
    if (remaining < 0) remaining = 0;
    std::string tail;
    for (int i = 0; i < remaining; i++) tail += "\u2500";
    return hbox({
        text(head) | color(TC("main_border")),
        text(title) | bold | color(TC("title")),
        text(" " + tail) | color(TC("main_border")),
    });
}

static Element infoRow(const std::string& label, const std::string& value, Color val_color) {
    return hbox({
        text("  " + label + ": ") | color(TC("dim")),
        text(value) | color(val_color),
        filler(),
    });
}

static Element permsLine(const std::string& perm, const std::string& name) {
    return hbox({
        text("  ") | color(TC("dim")),
        text(perm) | color(TC("syn_keyword")) | dim,
        text(" ") | color(TC("dim")),
        text(name) | color(TC("main_fg")),
    });
}

Element RenderFolderDetailsPanel(MainState& state, int tw, int th) {
    int pw = std::min(65, tw - 4);
    int ph = std::min(28, th - 4);
    int content_w = pw - 4;
    Elements els;

    els.push_back(makeSection("Details", content_w));
    els.push_back(separator() | color(TC("main_border")));

    if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
        fs::path fullPath = fs::path(state.currentPath) / state.filteredContents[state.selected];
        std::string name = state.filteredContents[state.selected];
        bool is_dir = FileManager::isDirectory(fullPath.string());

        els.push_back(infoRow("Name", name, TC("main_fg")));
        els.push_back(infoRow("Path", fullPath.string(), TC("syn_comment")));

        std::error_code ec;
        if (is_dir) {
            int fileCount = 0, folderCount = 0;
            std::vector<std::tuple<std::string, mode_t>> folderPermissions;
            std::vector<std::string> fileNames;
            FileManager::calculation_current_folder_files_number(
                fullPath.string(), fileCount, folderCount, folderPermissions, fileNames);

            int total = fileCount + folderCount;
            els.push_back(infoRow("Contents",
                std::to_string(total) + " items (" + std::to_string(fileCount) + " files, "
                + std::to_string(folderCount) + " dirs)", TC("main_fg")));

            auto dir_entry = fs::directory_entry(fullPath, ec);
            if (!ec) {
                els.push_back(infoRow("Modified", formatTime(dir_entry.last_write_time()), TC("main_fg")));
            }

            if (!folderPermissions.empty()) {
                els.push_back(text(""));
                els.push_back(makeSection("Subdirectories", content_w));
                for (const auto& [subname, mode] : folderPermissions) {
                    std::string perm = "d";
                    perm += (mode & S_IRUSR) ? 'r' : '-';
                    perm += (mode & S_IWUSR) ? 'w' : '-';
                    perm += (mode & S_IXUSR) ? 'x' : '-';
                    perm += (mode & S_IRGRP) ? 'r' : '-';
                    perm += (mode & S_IWGRP) ? 'w' : '-';
                    perm += (mode & S_IXGRP) ? 'x' : '-';
                    perm += (mode & S_IROTH) ? 'r' : '-';
                    perm += (mode & S_IWOTH) ? 'w' : '-';
                    perm += (mode & S_IXOTH) ? 'x' : '-';
                    els.push_back(permsLine(perm, subname));
                }
            }
        } else {
            // File details
            auto dir_entry = fs::directory_entry(fullPath, ec);
            if (!ec) {
                uintmax_t fsize = dir_entry.file_size();
                auto ftime = dir_entry.last_write_time();
                auto perms = fs::status(fullPath, ec).permissions();

                els.push_back(infoRow("Size", formatSize(fsize), TC("warning")));
                els.push_back(infoRow("Modified", formatTime(ftime), TC("main_fg")));

                bool is_sym = dir_entry.is_symlink();
                std::string type_str;
                if (is_sym) type_str = "symbolic link";
                else if (dir_entry.is_regular_file()) {
                    if (!fullPath.extension().empty())
                        type_str = fullPath.extension().string().substr(1) + " file";
                    else
                        type_str = "regular file";
                }
                els.push_back(infoRow("Type", type_str, TC("syn_keyword")));

                std::string perm;
                perm += is_sym ? 'l' : '-';
                perm += (perms & fs::perms::owner_read)  != fs::perms::none ? 'r' : '-';
                perm += (perms & fs::perms::owner_write) != fs::perms::none ? 'w' : '-';
                perm += (perms & fs::perms::owner_exec)  != fs::perms::none ? 'x' : '-';
                perm += (perms & fs::perms::group_read)  != fs::perms::none ? 'r' : '-';
                perm += (perms & fs::perms::group_write) != fs::perms::none ? 'w' : '-';
                perm += (perms & fs::perms::group_exec)  != fs::perms::none ? 'x' : '-';
                perm += (perms & fs::perms::others_read) != fs::perms::none ? 'r' : '-';
                perm += (perms & fs::perms::others_write)!= fs::perms::none ? 'w' : '-';
                perm += (perms & fs::perms::others_exec) != fs::perms::none ? 'x' : '-';
                els.push_back(infoRow("Permissions", perm, TC("syn_comment")));
            } else {
                els.push_back(text("  Unable to read file metadata") | color(TC("error")));
            }
        }
    } else {
        els.push_back(text("  No item selected") | color(TC("dim")));
    }

    els.push_back(text(""));
    els.push_back(text(" [Esc/q]=Close") | color(TC("dim")) | dim);

    return vbox(std::move(els)) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center | frame;
}

bool HandleFolderDetailsEvent(MainState& state, const Event& event) {
    if (event == Event::Escape || event == Event::Character('q')) {
        state.active_panel = ActivePanel::None;
        return true;
    }
    return true;
}

}  // namespace FTB::UI
