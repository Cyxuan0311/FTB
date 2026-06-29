#include "../include/browser/BinaryFileHandler.hpp"
#include <fstream>

namespace BinaryFileHandler {

const std::set<std::string> BinaryFileRestrictor::binaryExtensions = {
    // 可执行文件
    ".exe", ".dll", ".so", ".dylib", ".bin",
    // 视频文件 (.ts 已移除 — 与 TypeScript 冲突，改由内容检测区分)
    ".mp4", ".avi", ".mov", ".mkv", ".flv", ".wmv", ".webm", ".m4v", ".3gp",
    ".m2ts", ".mts", ".vob",
    // 音频文件
    ".mp3", ".wav", ".ogg", ".flac", ".aac", ".m4a", ".wma", ".aiff", ".alac", ".ape",
    // 图像文件
    ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".tif", ".svg", ".ico", ".webp",
    // 档案/压缩文件
    ".zip", ".tar", ".tgz", ".tbz2", ".txz", ".tzst", ".7z", ".rar", ".cab",
    // 其他二进制文件
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

bool BinaryFileRestrictor::isBinaryFile(const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (binaryExtensions.find(ext) != binaryExtensions.end()) return true;
    // 内容检测：对于文本疑点文件，检查前 4KB 是否有 NUL 字节
    if (filepath.has_filename() && std::filesystem::exists(filepath)
        && std::filesystem::is_regular_file(filepath)) {
        return hasNullBytes(filename);
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