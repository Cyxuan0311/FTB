#pragma once

#include <string>
#include <vector>
#include <set>
#include "../include/FTB/BinaryFileHandler.hpp"
#include <algorithm>
#include <filesystem>

namespace BinaryFileHandler {

class BinaryFileRestrictor {
public:
    static const std::set<std::string>& getBinaryExtensions();
    static bool isBinaryFile(const std::string& filename);
    static bool shouldRestrictPreview(const std::string& filename);
    static bool shouldRestrictEdit(const std::string& filename);
    
private:
    static const std::set<std::string> binaryExtensions;
};

} // namespace BinaryFileHandler