#pragma once

#include <string>
#include <functional>

enum class ArchiveFormat { Unknown, Zip, Tar, Gzip, Bzip2, Xz, Zstd, SevenZ, Rar, Cab, Iso };

ArchiveFormat DetectArchiveFormat(const std::string& filePath);
std::string FormatName(ArchiveFormat fmt);
bool IsExtractToolAvailable(ArchiveFormat fmt);
bool ExtractArchive(const std::string& filePath, const std::string& destDir,
                    std::function<void(const std::string&)> on_status,
                    std::function<bool()> is_cancelled);
