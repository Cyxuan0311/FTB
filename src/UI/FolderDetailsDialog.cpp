#include "../../include/UI/FolderDetailsDialog.hpp"
#include "../../include/FTB/FileManager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderFolderDetailsPanel(MainState& state, int tw, int th) {
    int pw = std::min(65, tw - 4);
    int ph = std::min(28, th - 4);
    Elements els;
    els.push_back(text(" Folder Details") | color(TC("title")) | bold);
    els.push_back(separator());
    if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
        fs::path fullPath = fs::path(state.currentPath) / state.filteredContents[state.selected];
        if (FileManager::isDirectory(fullPath.string())) {
            int fileCount = 0, folderCount = 0;
            std::vector<std::tuple<std::string, mode_t>> folderPermissions;
            std::vector<std::string> fileNames;
            FileManager::calculation_current_folder_files_number(
                fullPath.string(), fileCount, folderCount, folderPermissions, fileNames);
            els.push_back(hbox({ text(" Path: ") | color(TC("dim")), text(fullPath.string()) | color(TC("main_fg")) }));
            els.push_back(text(" Files: " + std::to_string(fileCount) + "  Folders: " + std::to_string(folderCount)) | color(TC("main_fg")));
            for (const auto& [name, mode] : folderPermissions) {
                std::string perm;
                perm += (mode & S_IRUSR) ? 'r' : '-';
                perm += (mode & S_IWUSR) ? 'w' : '-';
                perm += (mode & S_IXUSR) ? 'x' : '-';
                perm += (mode & S_IRGRP) ? 'r' : '-';
                perm += (mode & S_IWGRP) ? 'w' : '-';
                perm += (mode & S_IXGRP) ? 'x' : '-';
                perm += (mode & S_IROTH) ? 'r' : '-';
                perm += (mode & S_IWOTH) ? 'w' : '-';
                perm += (mode & S_IXOTH) ? 'x' : '-';
                els.push_back(text("  " + perm + " " + name) | color(TC("main_fg")));
            }
        }
    }
    els.push_back(text(""));
    els.push_back(text(" Esc=Close") | color(TC("dim")) | dim);
    return vbox(els) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleFolderDetailsEvent(MainState& state, const Event& event) {
    if (event == Event::Character('q')) {
        state.active_panel = ActivePanel::None;
        return true;
    }
    return true;
}

}  // namespace FTB::UI
