#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ArchiveEntry {
    std::string name;
    bool is_dir = false;
    uintmax_t size = 0;
};

bool IsArchiveFile(const std::string& name);
std::vector<ArchiveEntry> ListArchiveContents(const std::string& filePath);
