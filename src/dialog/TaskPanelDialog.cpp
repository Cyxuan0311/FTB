#include "dialog/TaskPanelDialog.hpp"
#include "browser/TaskSystem.hpp"
#include <iomanip>
#include <sstream>

namespace FTB::UI {

using namespace ftxui;

static std::string format_size(uintmax_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024ULL * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        return oss.str();
    }
    if (bytes < 1024ULL * 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return oss.str();
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    return oss.str();
}

static std::string format_speed(double bytes_per_sec) {
    if (bytes_per_sec < 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << bytes_per_sec << " B/s";
        return oss.str();
    }
    if (bytes_per_sec < 1024.0 * 1024.0) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes_per_sec / 1024.0) << " KB/s";
        return oss.str();
    }
    if (bytes_per_sec < 1024.0 * 1024.0 * 1024.0) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes_per_sec / (1024.0 * 1024.0)) << " MB/s";
        return oss.str();
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (bytes_per_sec / (1024.0 * 1024.0 * 1024.0)) << " GB/s";
    return oss.str();
}

static std::string format_eta(double bytes_per_sec, uintmax_t remaining) {
    if (bytes_per_sec <= 0.001) return "--:--";
    double secs = static_cast<double>(remaining) / bytes_per_sec;
    if (secs < 0) return "--:--";
    if (secs < 3600) {
        int m = static_cast<int>(secs) / 60;
        int s = static_cast<int>(secs) % 60;
        std::ostringstream oss;
        oss << m << ":" << std::setw(2) << std::setfill('0') << s;
        return oss.str();
    }
    int h = static_cast<int>(secs) / 3600;
    int m = (static_cast<int>(secs) % 3600) / 60;
    std::ostringstream oss;
    oss << h << ":" << std::setw(2) << std::setfill('0') << m;
    return oss.str();
}

static const char* state_icon(TaskState s) {
    switch (s) {
    case TaskState::Pending:    return "[-]";
    case TaskState::Running:    return "[>]";
    case TaskState::Paused:     return "[||]";
    case TaskState::Completed:  return "[v]";
    case TaskState::Failed:     return "[x]";
    case TaskState::Cancelled:  return "[!]";
    }
    return "[?]";
}

static Color state_color(TaskState s) {
    switch (s) {
    case TaskState::Running:    return TC("marker_copied");
    case TaskState::Paused:     return Color::GrayDark;
    case TaskState::Completed:  return Color::Green;
    case TaskState::Failed:     return TC("error");
    case TaskState::Cancelled:  return Color::GrayDark;
    default:                    return TC("main_fg");
    }
}

static Element build_task_entry(const TaskSnapshot& snap, bool selected) {
    Elements lines;

    float prog_ratio = 0.0f;
    if (snap.total_bytes > 0) {
        prog_ratio = std::min(1.0f, static_cast<float>(
            static_cast<double>(snap.bytes_processed) / snap.total_bytes));
    } else if (snap.total_files > 0) {
        prog_ratio = std::min(1.0f, static_cast<float>(
            static_cast<double>(snap.files_processed) / snap.total_files));
    }

    Color sc = state_color(snap.state);

    // Title line: [icon] title
    std::string type_str;
    switch (snap.type) {
    case TaskType::Copy:        type_str = "Copy"; break;
    case TaskType::Move:        type_str = "Move"; break;
    case TaskType::Delete:      type_str = "Delete"; break;
    case TaskType::Extract:     type_str = "Extract"; break;
    case TaskType::Rename:      type_str = "Rename"; break;
    case TaskType::BulkRename:  type_str = "Bulk Rename"; break;
    }
    auto title_text = text(" " + std::string(state_icon(snap.state))
                           + " " + type_str + ": " + snap.title + "  ")
        | color(sc);

    if (snap.state == TaskState::Completed) {
        title_text = title_text | bold;
    }

    lines.push_back(title_text);

    // Progress bar and stats
    auto gauge_elem = gauge(prog_ratio)
        | flex_grow
        | color(TC("gauge_fill"))
        | bgcolor(TC("gauge_bg"));

    std::string prog_str;
    if (snap.total_bytes > 0) {
        prog_str = format_size(snap.bytes_processed) + "/" + format_size(snap.total_bytes);
    } else if (snap.total_files > 0) {
        prog_str = std::to_string(snap.files_processed) + "/" + std::to_string(snap.total_files) + " files";
    } else {
        prog_str = std::to_string(snap.files_processed) + " items";
    }

    std::string pct_str = std::to_string(static_cast<int>(prog_ratio * 100.0)) + "%";

    auto stat_text = text(" " + prog_str + "  " + pct_str) | color(TC("main_fg"));

    Element bottom;
    if (snap.state == TaskState::Running) {
        uintmax_t remaining = (snap.total_bytes > snap.bytes_processed)
            ? snap.total_bytes - snap.bytes_processed : 0;
        std::string info = " Speed: " + format_speed(snap.current_speed)
                         + "  ETA: " + format_eta(snap.current_speed, remaining);
        auto info_text = text(info) | color(TC("dim"));
        bottom = hbox({
            gauge_elem,
            separatorEmpty(),
            info_text,
        });
    } else if (snap.state == TaskState::Paused) {
        bottom = hbox({
            gauge_elem | dim,
            separatorEmpty(),
            text(" PAUSED") | color(Color::GrayDark) | bold,
        });
    } else {
        bottom = hbox({
            gauge_elem | dim,
            separatorEmpty(),
            stat_text,
        });
    }

    lines.push_back(bottom);

    auto entry = vbox(std::move(lines));

    if (selected) {
        entry = entry | bgcolor(TC("find_keyword"));
    }

    return entry | flex_grow;
}

Element RenderTaskPanel(MainState& state, int tw, int th) {
    auto& ts = TaskSystem::getInstance();
    auto snapshots = ts.get_snapshot();

    int pw = std::min(65, tw - 4);

    Elements header_els;
    header_els.push_back(text(" Tasks (" + std::to_string(snapshots.size()) + ")")
        | color(TC("title")) | bold);
    header_els.push_back(separator());

    if (snapshots.empty()) {
        header_els.push_back(text("  (no tasks)") | color(TC("dim")));
    } else {
        int total = static_cast<int>(snapshots.size());
        int scroll = state.panel_selected;
        if (scroll < 0) scroll = 0;
        if (scroll >= total) scroll = total - 1;

        int max_visible = std::max(3, th - 8);
        int start = std::max(0, scroll - max_visible / 2);
        int end = std::min(total, start + max_visible);

        for (int i = start; i < end; ++i) {
            header_els.push_back(build_task_entry(snapshots[i], i == scroll));
            if (i < end - 1) {
                header_els.push_back(separatorEmpty());
            }
        }

        if (total > max_visible) {
            header_els.push_back(text("  " + std::to_string(start + 1) + "-"
                + std::to_string(end) + "/" + std::to_string(total))
                | color(TC("dim")) | dim);
        }
    }

    header_els.push_back(separator());
    header_els.push_back(
        text(" [x]=Cancel  [Space]=Pause/Resume  j/k=Scroll  Esc=Close")
        | color(TC("dim")) | dim);

    return vbox(std::move(header_els)) | bgcolor(TC("main_bg")) | GetPanelBorder()
           | size(WIDTH, EQUAL, pw) | center;
}

bool HandleTaskPanelEvent(MainState& state, const Event& event) {
    auto& ts = TaskSystem::getInstance();
    auto snapshots = ts.get_snapshot();
    int total = static_cast<int>(snapshots.size());

    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_selected = 0;
        return true;
    }

    if (total == 0) return true;

    // Select task
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) --state.panel_selected;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < total - 1) ++state.panel_selected;
        return true;
    }

    // Cancel selected task
    if (event == Event::Character('x') || event == Event::Delete) {
        if (state.panel_selected >= 0 && state.panel_selected < total) {
            const auto& snap = snapshots[state.panel_selected];
            if (snap.state == TaskState::Running || snap.state == TaskState::Paused) {
                ts.cancel(snap.id);
            }
        }
        return true;
    }

    // Pause/Resume selected task
    if (event == Event::Character(' ') || event == Event::Character('p')) {
        if (state.panel_selected >= 0 && state.panel_selected < total) {
            const auto& snap = snapshots[state.panel_selected];
            if (snap.state == TaskState::Running) {
                ts.pause(snap.id);
            } else if (snap.state == TaskState::Paused) {
                ts.resume(snap.id);
            }
        }
        return true;
    }

    return true;
}

}  // namespace FTB::UI
