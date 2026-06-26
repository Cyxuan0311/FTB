#include "browser/SortMode.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace FTB {

static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

static std::string getExtension(const std::string& name) {
    auto dot = name.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return "";
    return toLower(name.substr(dot));
}

SortMode SortModeFromString(const std::string& str) {
    if (str == "name_desc")  return SortMode::NameDesc;
    if (str == "size_asc")   return SortMode::SizeAsc;
    if (str == "size_desc")  return SortMode::SizeDesc;
    if (str == "time_asc")   return SortMode::TimeAsc;
    if (str == "time_desc")  return SortMode::TimeDesc;
    if (str == "type")       return SortMode::Type;
    if (str == "extension")  return SortMode::Extension;
    return SortMode::NameAsc;
}

std::string SortModeToString(SortMode mode) {
    switch (mode) {
    case SortMode::NameAsc:    return "name_asc";
    case SortMode::NameDesc:   return "name_desc";
    case SortMode::SizeAsc:    return "size_asc";
    case SortMode::SizeDesc:   return "size_desc";
    case SortMode::TimeAsc:    return "time_asc";
    case SortMode::TimeDesc:   return "time_desc";
    case SortMode::Type:       return "type";
    case SortMode::Extension:  return "extension";
    }
    return "name_asc";
}

std::string SortModeDescription(SortMode mode) {
    switch (mode) {
    case SortMode::NameAsc:    return "Name (A-Z)";
    case SortMode::NameDesc:   return "Name (Z-A)";
    case SortMode::SizeAsc:    return "Size (smallest first)";
    case SortMode::SizeDesc:   return "Size (largest first)";
    case SortMode::TimeAsc:    return "Time (oldest first)";
    case SortMode::TimeDesc:   return "Time (newest first)";
    case SortMode::Type:       return "Type (group by extension)";
    case SortMode::Extension:  return "Extension (alphabetical)";
    }
    return "";
}

std::vector<SortMode> GetAllSortModes() {
    return {
        SortMode::NameAsc,
        SortMode::NameDesc,
        SortMode::SizeAsc,
        SortMode::SizeDesc,
        SortMode::TimeAsc,
        SortMode::TimeDesc,
        SortMode::Type,
        SortMode::Extension,
    };
}

void SortEntries(std::vector<FileManager::DirEntryInfo>& entries, SortMode mode) {
    const auto& cmp = [mode](const FileManager::DirEntryInfo& a, const FileManager::DirEntryInfo& b) -> bool {
        if (a.is_dir != b.is_dir) return a.is_dir;

        switch (mode) {
        case SortMode::NameAsc: {
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::NameDesc: {
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na > nb;
        }
        case SortMode::SizeAsc: {
            if (a.is_dir) {
                std::string na = toLower(a.name), nb = toLower(b.name);
                return na < nb;
            }
            if (a.file_size != b.file_size) return a.file_size < b.file_size;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::SizeDesc: {
            if (a.is_dir) {
                std::string na = toLower(a.name), nb = toLower(b.name);
                return na < nb;
            }
            if (a.file_size != b.file_size) return a.file_size > b.file_size;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::TimeAsc: {
            auto cmp_time = a.mod_time.compare(b.mod_time);
            if (cmp_time != 0) return cmp_time < 0;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::TimeDesc: {
            auto cmp_time = a.mod_time.compare(b.mod_time);
            if (cmp_time != 0) return cmp_time > 0;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::Type: {
            std::string ext_a = getExtension(a.name);
            std::string ext_b = getExtension(b.name);
            if (ext_a != ext_b) return ext_a < ext_b;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        case SortMode::Extension: {
            std::string ext_a = getExtension(a.name);
            std::string ext_b = getExtension(b.name);
            if (ext_a != ext_b) return ext_a < ext_b;
            std::string na = toLower(a.name), nb = toLower(b.name);
            return na < nb;
        }
        }
        return false;
    };

    std::sort(entries.begin(), entries.end(), cmp);
}

} // namespace FTB
