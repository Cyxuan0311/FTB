#pragma once
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace FTB {

inline bool renameWithFallback(const fs::path& from, const fs::path& to) {
    std::error_code ec;
    fs::rename(from, to, ec);
    if (!ec) return true;

    if (ec == std::errc::cross_device_link) {
        std::error_code cp_ec, rm_ec;
        if (fs::is_directory(from)) {
            fs::copy(from, to,
                fs::copy_options::recursive | fs::copy_options::overwrite_existing, cp_ec);
            if (cp_ec) return false;
            fs::remove_all(from, rm_ec);
        } else {
            fs::copy_file(from, to, fs::copy_options::overwrite_existing, cp_ec);
            if (cp_ec) return false;
            fs::remove(from, rm_ec);
        }
        return !rm_ec;
    }

    return false;
}

} // namespace FTB
