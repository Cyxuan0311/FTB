#include "../../include/dialog/FilePreviewDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderFilePreviewPanel(MainState& state, int tw, int th) {
    int pw = std::min(65, tw - 4);
    int ph = std::min(32, th - 4);
    std::string content;
    std::string filename;
    if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
        fs::path fullPath = fs::path(state.currentPath) / state.filteredContents[state.selected];
        filename = fullPath.string();
        if (fs::is_regular_file(fullPath)) {
            content = FileManager::readFileContent(fullPath.string(), 1, ph - 4);
        }
    }
    Elements lines;
    lines.push_back(text(" Preview: " + filename) | color(TC("title")) | bold);
    lines.push_back(separator());
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(text(" " + line) | color(TC("main_fg")));
    }
    lines.push_back(text(""));
    lines.push_back(text(" Esc=Close") | color(TC("dim")) | dim);
    return vbox(lines) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

bool HandleFilePreviewEvent(MainState& state, const Event& event) {
    if (event == Event::Character('q')) {
        state.active_panel = ActivePanel::None;
        return true;
    }
    return true;
}

}  // namespace FTB::UI
