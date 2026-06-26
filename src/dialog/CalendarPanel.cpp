#include "../../include/dialog/CalendarPanel.hpp"
#include "../../include/config/ThemeManager.hpp"
#include "../../include/config/ConfigManager.hpp"

#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace FTB::UI {

using namespace ftxui;

// ─── Calendar state ──────────────────────────────────────────────────
static int cal_year = 0;
static int cal_month = 0;
static bool cal_initialized = false;

static void InitCalendarState() {
    if (!cal_initialized) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_c);
        cal_year = 1900 + tm.tm_year;
        cal_month = tm.tm_mon + 1;
        cal_initialized = true;
    }
}

static int DaysInMonth(int year, int month) {
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
        case 4: case 6: case 9: case 11: return 30;
        case 2: return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28;
        default: return 30;
    }
}

static int FirstDayOfWeek(int year, int month) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = 1;
    std::mktime(&tm);
    return tm.tm_wday;  // 0=Sun
}

static std::string MonthName(int month) {
    static const char* names[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    return names[(month - 1) % 12];
}

// ─── Render ──────────────────────────────────────────────────────────

Element RenderCalendarPanel(MainState& /*state*/, int tw, int /*th*/) {
    InitCalendarState();

    int pw = std::min(38, tw - 4);

    // Get today
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);
    int today_day = now_tm.tm_mday;
    int today_month = now_tm.tm_mon + 1;
    int today_year = 1900 + now_tm.tm_year;

    bool is_current_month = (cal_year == today_year && cal_month == today_month);

    Color accent = TC("syn_keyword");
    Color dim_c = TC("dim");
    Color fg = TC("main_fg");
    Color weekend_c = TC("syn_comment");

    Elements content;

    // ── Month/Year navigation header ──
    std::string title_text = " " + MonthName(cal_month) + " " + std::to_string(cal_year) + " ";
    content.push_back(
        hbox({
            text(" h/L:prev") | color(dim_c),
            filler(),
            text(title_text) | color(accent) | bold,
            filler(),
            text("l/H:next ") | color(dim_c),
        })
    );

    content.push_back(separator() | color(TC("main_border")));

    // ── Weekday header ──
    Elements week_header;
    const char* day_names[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int i = 0; i < 7; ++i) {
        Color dc = (i == 0 || i == 6) ? weekend_c : dim_c;
        week_header.push_back(text(std::string(" ") + day_names[i] + " ") | color(dc));
    }
    content.push_back(hbox(week_header));

    // ── Day grid ──
    int first_dow = FirstDayOfWeek(cal_year, cal_month);
    int days = DaysInMonth(cal_year, cal_month);

    Elements week_row;
    // Leading blanks
    for (int i = 0; i < first_dow; ++i) {
        week_row.push_back(text("   "));
    }

    for (int day = 1; day <= days; ++day) {
        std::string day_str = (day < 10 ? " " : "") + std::to_string(day);
        int dow = (first_dow + day - 1) % 7;
        bool is_weekend = (dow == 0 || dow == 6);
        bool is_today = is_current_month && (day == today_day);

        Element day_el = text(day_str);

        if (is_today) {
            day_el = day_el | bgcolor(accent) | color(TC("main_bg")) | bold;
        } else if (is_weekend) {
            day_el = day_el | color(weekend_c);
        } else {
            day_el = day_el | color(fg);
        }

        week_row.push_back(hbox({day_el, text(" ")}));

        if ((first_dow + day) % 7 == 0 || day == days) {
            // Pad remaining cells
            int remaining = 7 - static_cast<int>(week_row.size());
            for (int r = 0; r < remaining; ++r) {
                week_row.push_back(text("   "));
            }
            content.push_back(hbox(week_row));
            week_row.clear();
        }
    }

    content.push_back(separator() | color(TC("main_border")));

    // ── Footer: current time ──
    std::ostringstream time_str;
    time_str << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    std::string weekday_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    content.push_back(
        hbox({
            text(" " + weekday_names[now_tm.tm_wday]) | color(accent) | bold,
            text("  " + time_str.str()) | color(fg),
        })
    );

    content.push_back(text(" h/l:month  H/L:year  r:today  q:close") | color(dim_c) | dim);

    return vbox({
        text(" Calendar") | color(TC("title")) | bold,
        separator() | color(TC("main_border")),
        vbox(content) | flex,
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
       size(WIDTH, EQUAL, pw) | center;
}

// ─── Event handling ──────────────────────────────────────────────────

bool HandleCalendarEvent(MainState& state, const Event& event) {
    InitCalendarState();

    if (event == Event::Character('q') || event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }

    // h: previous month
    if (event == Event::Character('h')) {
        cal_month--;
        if (cal_month < 1) { cal_month = 12; cal_year--; }
        return true;
    }

    // l: next month
    if (event == Event::Character('l')) {
        cal_month++;
        if (cal_month > 12) { cal_month = 1; cal_year++; }
        return true;
    }

    // H: previous year
    if (event == Event::Character('H')) {
        cal_year--;
        return true;
    }

    // L: next year
    if (event == Event::Character('L')) {
        cal_year++;
        return true;
    }

    // r: reset to today
    if (event == Event::Character('r')) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_c);
        cal_year = 1900 + tm.tm_year;
        cal_month = tm.tm_mon + 1;
        return true;
    }

    return true;
}

}  // namespace FTB::UI
