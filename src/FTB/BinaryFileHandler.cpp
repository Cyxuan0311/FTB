#include "../include/FTB/BinaryFileHandler.hpp"

namespace BinaryFileHandler {

const std::set<std::string> BinaryFileRestrictor::binaryExtensions = {
    // 可执行文件
    ".exe", ".dll", ".so", ".dylib", ".bin",
    // 视频文件
    ".mp4", ".avi", ".mov", ".mkv", ".flv", ".wmv", ".webm", ".m4v", ".3gp", ".ts",
    // 音频文件
    ".mp3", ".wav", ".ogg", ".flac", ".aac", ".m4a", ".wma", ".aiff", ".alac", ".ape",
    // 图像文件
    ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".tif", ".svg", ".ico", ".webp",
    // 其他二进制文件
    ".iso", ".img", ".dat", ".bin", ".obj",".a"
};

bool BinaryFileRestrictor::isBinaryFile(const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return binaryExtensions.find(ext) != binaryExtensions.end();
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