#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct FdResult {
    std::string path;
    bool is_dir = false;
};

bool FuzzyMatch(const std::string& query, const std::string& str);

class SearchEngine {
public:
    static bool HasExternal();
    static std::vector<FdResult> Search(const std::string& query,
                                        const std::string& basePath);

private:
    static std::vector<FdResult> ExternalSearch(const std::string& query,
                                                 const std::string& basePath);
    static std::vector<FdResult> BuiltinSearch(const std::string& query,
                                                const std::string& basePath);
};
