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

// Unicode state icons (yazi-style)
static const char* state_icon(TaskState s) {
    switch (s) {
    case TaskState::Pending:    return " \u25CB";   // ○
    case TaskState::Running:    return " \u25B6";   // ▶
    case TaskState::Paused:     return " \u23F8";   // ⏸
    case TaskState::Completed:  return " \u2713";   // ✓
    case TaskState::Failed:     return " \u2715";   // ✕
    case TaskState::Cancelled:  return " \u2298";   // ⊘
    }
    return " ?";
}

static Color state_color(TaskState s) {
    switch (s) {
    case TaskState::Running:    return TC("marker_copied");
    case TaskState::Paused:     return Color::GrayDark;
    case TaskState::Completed:  return Color::Green;
    case TaskState::Failed:     return TC("error");
    case TaskState::Cancelled:  return Color::GrayDark;
    default:                    return TC("dialog_fg");
    }
}

static std::string task_type_str(TaskType t) {
    switch (t) {
    case TaskType::Copy:        return "Copy";
    case TaskType::Move:        return "Move";
    case TaskType::Delete:      return "Delete";
    case TaskType::Extract:     return "Extract";
    case TaskType::Rename:      return "Rename";
    case TaskType::BulkRename:  return "Bulk Rename";
    }
    return "?";
}

static Element build_task_entry(const TaskSnapshot& snap, bool selected) {
    float prog_ratio = 0.0f;
    if (snap.total_bytes > 0) {
        prog_ratio = std::min(1.0f, static_cast<float>(
            static_cast<double>(snap.bytes_processed) / snap.total_bytes));
    } else if (snap.total_files > 0) {
        prog_ratio = std::min(1.0f, static_cast<float>(
            static_cast<double>(snap.files_processed) / snap.total_files));
    }

    Color sc = state_color(snap.state);
    std::string type = task_type_str(snap.type);

    // ── Line 1: state icon + type + title ──
    Elements line1_parts = {
        text(" "),
        text(state_icon(snap.state)) | color(sc),
        text(" ") | color(TC("dialog_fg")),
        text(type) | color(TC("dialog_fg")) | bold,
        text(": " + snap.title) | color(TC("dialog_fg")),
        filler(),
    };

    // Right-aligned state label for completed/failed/cancelled/paused
    std::string label;
    Color label_color = sc;
    if (snap.state == TaskState::Completed) {
        label = " DONE";
        label_color = Color::Green;
    } else if (snap.state == TaskState::Failed) {
        label = " FAILED";
        label_color = TC("error");
    } else if (snap.state == TaskState::Cancelled) {
        label = " CANCELLED";
        label_color = Color::GrayDark;
    } else if (snap.state == TaskState::Paused) {
        label = " PAUSED";
        label_color = Color::GrayDark;
    }
    if (!label.empty()) {
        line1_parts.push_back(text(label) | color(label_color) | bold);
        line1_parts.push_back(text(" "));
    }

    Element line1 = hbox(std::move(line1_parts));

    // ── Line 2: progress bar + percentage ──
    std::string pct_str = std::to_string(static_cast<int>(prog_ratio * 100.0)) + "%";

    Element gauge_elem;
    if (snap.state == TaskState::Completed) {
        gauge_elem = gauge(1.0f) | flex_grow | color(Color::Green) | bgcolor(TC("dialog_bg"));
    } else if (snap.state == TaskState::Failed || snap.state == TaskState::Cancelled) {
        gauge_elem = gauge(prog_ratio) | flex_grow | color(TC("error")) | bgcolor(TC("dialog_bg")) | dim;
    } else {
        gauge_elem = gauge(prog_ratio) | flex_grow | color(TC("gauge_fill")) | bgcolor(TC("dialog_bg"));
        if (snap.state == TaskState::Paused) {
            gauge_elem = gauge_elem | dim;
        }
    }

    Element line2 = hbox({
        text("  "),
        gauge_elem,
        text(" ") | color(TC("dialog_fg")),
        text(pct_str) | color(TC("dialog_fg")) | bold,
        text(" "),
    });

    // ── Line 3: progress stats + speed + ETA ──
    std::string prog_str;
    if (snap.total_bytes > 0) {
        prog_str = format_size(snap.bytes_processed) + " / " + format_size(snap.total_bytes);
    } else if (snap.total_files > 0) {
        prog_str = std::to_string(snap.files_processed) + " / " + std::to_string(snap.total_files) + " files";
    } else {
        prog_str = std::to_string(snap.files_processed) + " items";
    }

    Element line3;
    if (snap.state == TaskState::Running) {
        uintmax_t remaining = (snap.total_bytes > snap.bytes_processed)
            ? snap.total_bytes - snap.bytes_processed : 0;
        line3 = hbox({
            text("  "),
            text(prog_str) | color(TC("dialog_fg")),
            filler(),
            text(format_speed(snap.current_speed)) | color(TC("dim")),
            text("  "),
            text("ETA " + format_eta(snap.current_speed, remaining)) | color(TC("dim")),
            text(" "),
        });
    } else {
        line3 = hbox({
            text("  "),
            text(prog_str) | color(TC("dim")),
            filler(),
        });
    }

    // ── Assemble entry ──
    auto entry = vbox({
        line1,
        line2,
        line3,
    });

    if (selected) {
        entry = entry | bgcolor(TC("selection_bg"));
    }

    return entry | flex_grow;
}

Element RenderTaskPanel(MainState& state, int tw, int th) {
    auto& ts = TaskSystem::getInstance();
    auto snapshots = ts.get_snapshot();

    int pw = std::min(72, tw - 4);

    Elements els;

    // ── Header ──
    els.push_back(hbox({
        text(" Tasks") | color(TC("title")) | bold,
        text(" (" + std::to_string(snapshots.size()) + ")") | color(TC("dim")),
        filler(),
    }));
    els.push_back(separator() | color(TC("dialog_border")));

    // ── Body ──
    if (snapshots.empty()) {
        els.push_back(text(""));
        els.push_back(hbox({
            text("  "),
            text("(no tasks)") | color(TC("dim")) | dim,
        }));
        els.push_back(text(""));
    } else {
        int total = static_cast<int>(snapshots.size());
        int scroll = state.panel_selected;
        if (scroll < 0) scroll = 0;
        if (scroll >= total) scroll = total - 1;

        int max_visible = std::max(1, (th - 10) / 4);
        int start = std::max(0, scroll - max_visible / 2);
        int end = std::min(total, start + max_visible);

        for (int i = start; i < end; ++i) {
            els.push_back(build_task_entry(snapshots[i], i == scroll));
            if (i < end - 1) {
                els.push_back(text(""));  // spacer between tasks
            }
        }

        if (total > max_visible) {
            els.push_back(hbox({
                filler(),
                text(" " + std::to_string(start + 1) + "-"
                     + std::to_string(end) + "/" + std::to_string(total) + " ")
                    | color(TC("dim")) | dim,
            }));
        }
    }

    // ── Footer ──
    els.push_back(separator() | color(TC("dialog_border")));
    els.push_back(hbox({
        text(" x cancel") | color(TC("dim")) | dim,
        text("    space pause") | color(TC("dim")) | dim,
        text("    jk scroll") | color(TC("dim")) | dim,
        text("    esc close") | color(TC("dim")) | dim,
        filler(),
    }));

    return vbox(std::move(els))
        | bgcolor(TC("dialog_bg"))
        | GetPanelBorder()
        | size(WIDTH, EQUAL, pw)
        | center;
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

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) --state.panel_selected;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < total - 1) ++state.panel_selected;
        return true;
    }

    if (event == Event::Character('x') || event == Event::Delete) {
        if (state.panel_selected >= 0 && state.panel_selected < total) {
            const auto& snap = snapshots[state.panel_selected];
            if (snap.state == TaskState::Running || snap.state == TaskState::Paused) {
                ts.cancel(snap.id);
            }
        }
        return true;
    }

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
