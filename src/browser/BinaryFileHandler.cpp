#include "../include/browser/BinaryFileHandler.hpp"
#include <fstream>

namespace BinaryFileHandler {

const std::set<std::string> BinaryFileRestrictor::binaryExtensions = {
    ".exe", ".dll", ".so", ".dylib", ".bin",
    ".mp4", ".avi", ".mov", ".mkv", ".flv", ".wmv", ".webm", ".m4v", ".3gp",
    ".m2ts", ".mts", ".vob",
    ".mp3", ".wav", ".ogg", ".flac", ".aac", ".m4a", ".wma", ".aiff", ".alac", ".ape",
    ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".tif", ".svg", ".ico", ".webp",
    ".zip", ".tar", ".tgz", ".tbz2", ".txz", ".tzst", ".7z", ".rar", ".cab",
    ".iso", ".img", ".dat", ".bin", ".obj",".a"
};

static bool hasNullBytes(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    char buf[4096];
    file.read(buf, sizeof(buf));
    std::streamsize n = file.gcount();
    for (std::streamsize i = 0; i < n; ++i) {
        if (buf[i] == '\0') return true;
    }
    return false;
}

static bool hasBinaryContent(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    char buf[8192];
    file.read(buf, sizeof(buf));
    std::streamsize n = file.gcount();
    if (n == 0) return false;

    int control_chars = 0;
    for (std::streamsize i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(buf[i]);
        if (c == '\0') continue;
        if (c == '\t' || c == '\n' || c == '\r' || c == 0x1B) continue;
        if (c >= 0x20 && c <= 0x7E) continue;
        if (c >= 0x80) continue;
        control_chars++;
    }
    return static_cast<double>(control_chars) / n > 0.01;
}

bool BinaryFileRestrictor::isBinaryFile(const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (binaryExtensions.find(ext) != binaryExtensions.end()) return true;
    if (filepath.has_filename() && std::filesystem::exists(filepath)
        && std::filesystem::is_regular_file(filepath)) {
        if (hasNullBytes(filename)) return true;
        if (hasBinaryContent(filename)) return true;
    }
    return false;
}

bool BinaryFileRestrictor::shouldRestrictPreview(const std::string& filename) {
    return isBinaryFile(filename);
}

bool BinaryFileRestrictor::shouldRestrictEdit(const std::string& filename) {
    return isBinaryFile(filename);
}

const std::set<std::string>& BinaryFileRestrictor::getBinaryExtensions() {
    return binaryExtensions;
}

} // namespace BinaryFileHandler
