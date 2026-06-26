#pragma once

#include <string>
#include <vector>
#include "browser/FileManager.hpp"

namespace FTB {

enum class SortMode {
    NameAsc,
    NameDesc,
    SizeAsc,
    SizeDesc,
    TimeAsc,
    TimeDesc,
    Type,
    Extension,
};

SortMode SortModeFromString(const std::string& str);
std::string SortModeToString(SortMode mode);
std::string SortModeDescription(SortMode mode);
std::vector<SortMode> GetAllSortModes();

void SortEntries(std::vector<FileManager::DirEntryInfo>& entries, SortMode mode);

} // namespace FTB
