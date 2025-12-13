#include "FTB/IconMapper.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <vector>

namespace FTB {
namespace Icons {
namespace {

using IconMap = std::unordered_map<std::string, std::string>;

constexpr const char* DEFAULT_FOLDER_ICON = u8"\uf115 ";
constexpr const char* DEFAULT_FILE_ICON   = u8"\uf15b ";

const IconMap kFolderIcons = {
    {"src", u8"\ue5fe "},        // source folder
    {"include", u8"\uf0fd "},    // headers
    {"build", u8"\uf0ad "},      // tools
    {"cmake", u8"\ue615 "},
    {"scripts", u8"\ue795 "},
    {"assets", u8"\uf1b3 "},
    {"res", u8"\uf1b3 "},
    {"resources", u8"\uf1b3 "},
    {"config", u8"\uf013 "},
    {"configs", u8"\uf013 "},
    {"doc", u8"\uf02d "},
    {"docs", u8"\uf02d "},
    {"tests", u8"\uf0ae "},
    {"test", u8"\uf0ae "},
    {"examples", u8"\uf0c3 "},
    {"example", u8"\uf0c3 "},
    {"dist", u8"\uf0ee "},
    {"release", u8"\uf0ee "},
    {"bin", u8"\uf0e7 "},
    {"lib", u8"\uf121 "},
    {"vendor", u8"\uf7d9 "},
    {"third_party", u8"\uf7d9 "},
    {"public", u8"\uf1ad "},
    {"static", u8"\uf74e "},
    {".git", u8"\ue5fb "},
    {".github", u8"\ue5fb "},
    {".vscode", u8"\ue70c "},
    {".idea", u8"\ue7b5 "},
};

const std::vector<std::pair<std::string, std::string>> kFolderKeywordIcons = {
    {"test", u8"\uf24e "},
    {"spec", u8"\uf24e "},
    {"doc", u8"\uf02d "},
    {"sample", u8"\uf0c3 "},
    {"demo", u8"\uf0c3 "},
    {"script", u8"\ue795 "},
    {"tool", u8"\uf0ad "},
    {"asset", u8"\uf1b3 "},
    {"resource", u8"\uf1b3 "},
    {"config", u8"\uf013 "},
    {"setting", u8"\uf013 "},
    {"template", u8"\uf249 "},
    {"cache", u8"\uf0a0 "},
    {"tmp", u8"\uf0a0 "},
    {"log", u8"\uf24a "},
};

const IconMap kExactFileIcons = {
    {"cmakelists.txt", u8"\ue615 "},
    {"makefile", u8"\uf489 "},
    {"gnumakefile", u8"\uf489 "},
    {"readme.md", u8"\uf48a "},
    {"readme", u8"\uf48a "},
    {"license", u8"\uf02d "},
    {".gitignore", u8"\ue7a1 "},
    {"dockerfile", u8"\uf308 "},
    {"package.json", u8"\ue60f "},
    {"package-lock.json", u8"\ue615 "},
    {"yarn.lock", u8"\ue718 "},
    {"pnpm-lock.yaml", u8"\ue718 "},
    {"go.mod", u8"\ue626 "},
    {"go.sum", u8"\ue626 "},
    {"requirements.txt", u8"\uf81f "},
    {"pyproject.toml", u8"\uf81f "},
    {"tsconfig.json", u8"\ue628 "},
    {"babel.config.js", u8"\ue74e "},
    {"webpack.config.js", u8"\ue74e "},
    {"changelog", u8"\uf0f6 "},
    {"changelog.md", u8"\uf0f6 "},
    {"todo", u8"\uf0ae "},
    {"todo.md", u8"\uf0ae "},
    {"angular.json", u8"\uf7b6 "},
    {"composer.json", u8"\uf41d "},
    {"poetry.lock", u8"\uf81f "},
    {"cargo.toml", u8"\ue7a8 "},
    {"cargo.lock", u8"\ue7a8 "},
};

const IconMap kExtensionIcons = {
    // 源码
    {".cpp", u8"\ue61d "},  // nf-dev-cplusplus
    {".cc", u8"\ue61d "},
    {".cxx", u8"\ue61d "},
    {".ixx", u8"\ue61d "},
    {".c", u8"\ue61e "},    // nf-dev-c
    {".hpp", u8"\uf0fd "},  // nf-fa-file_text_o
    {".h", u8"\uf0fd "},
    {".hh", u8"\uf0fd "},
    {".hxx", u8"\uf0fd "},
    {".py", u8"\uf81f "},   // nf-md-language_python
    {".js", u8"\ue74e "},   // nf-dev-javascript
    {".ts", u8"\ue628 "},   // nf-dev-typescript
    {".tsx", u8"\ue7ba "},
    {".jsx", u8"\ue7ba "},
    {".go", u8"\ue627 "},   // nf-dev-go
    {".rs", u8"\ue7a8 "},   // nf-dev-rust
    {".java", u8"\ue256 "}, // nf-dev-java
    {".kt", u8"\ue634 "},   // Kotlin
    {".swift", u8"\ufbe3 "},
    {".cs", u8"\ue61a "},   // C#
    {".m", u8"\ue71e "},    // Objective-C
    {".mm", u8"\ue71e "},
    {".lua", u8"\ue620 "},
    {".hs", u8"\ue777 "},
    {".fs", u8"\ue7a7 "},
    {".fsx", u8"\ue7a7 "},
    {".fsi", u8"\ue7a7 "},
    {".clj", u8"\ue768 "},
    {".cljs", u8"\ue76a "},
    {".coffee", u8"\uf0f4 "},
    {".groovy", u8"\ue775 "},
    {".scala", u8"\ue737 "},
    {".nim", u8"\uf6a4 "},
    {".jl", u8"\uf640 "},
    {".vala", u8"\ue6b4 "},
    {".hx", u8"\ue60e "},
    {".asm", u8"\uf471 "},
    {".sqlx", u8"\uf1c0 "},

    // 标记 & 配置
    {".json", u8"\ufb25 "},
    {".yml", u8"\ufb25 "},
    {".yaml", u8"\ufb25 "},
    {".env", u8"\uf462 "},
    {".xml", u8"\ufabf "},
    {".toml", u8"\ufabf "},
    {".ini", u8"\uf83d "},
    {".conf", u8"\uf83d "},
    {".md", u8"\uf48a "},
    {".markdown", u8"\uf48a "},
    {".rst", u8"\uf48a "},
    {".cmake", u8"\ue615 "},
    {".gradle", u8"\ue73f "},
    {".sln", u8"\ue70c "},
    {".props", u8"\ue70c "},
    {".targets", u8"\ue70c "},
    {".lock", u8"\ue618 "},
    {".babelrc", u8"\ue74e "},
    {".eslintrc", u8"\ue74e "},
    {".stylelintrc", u8"\ue749 "},
    {".prettierrc", u8"\ue60f "},
    {".dockerignore", u8"\uf308 "},
    {".editorconfig", u8"\ue615 "},

    // 文档
    {".txt", u8"\uf15c "},
    {".pdf", u8"\uf1c1 "},
    {".doc", u8"\uf1c2 "},
    {".docx", u8"\uf1c2 "},
    {".xls", u8"\uf1c3 "},
    {".xlsx", u8"\uf1c3 "},
    {".ppt", u8"\uf1c4 "},
    {".pptx", u8"\uf1c4 "},
    {".odt", u8"\uf1c2 "},
    {".ods", u8"\uf1c3 "},
    {".odp", u8"\uf1c4 "},
    {".epub", u8"\uf02d "},
    {".rtf", u8"\uf15c "},
    {".pages", u8"\uf15c "},

    // 资源
    {".png", u8"\uf1c5 "},
    {".jpg", u8"\uf1c5 "},
    {".jpeg", u8"\uf1c5 "},
    {".gif", u8"\uf1c5 "},
    {".svg", u8"\uf1c5 "},
    {".bmp", u8"\uf1c5 "},
    {".ico", u8"\uf1c5 "},
    {".mp4", u8"\uf03d "},
    {".mov", u8"\uf03d "},
    {".mkv", u8"\uf03d "},
    {".mp3", u8"\uf001 "},
    {".wav", u8"\uf001 "},
    {".flac", u8"\uf001 "},
    {".ogg", u8"\uf001 "},
    {".psd", u8"\uf1c5 "},
    {".ai", u8"\uf1c5 "},
    {".blend", u8"\uf1c5 "},
    {".fbx", u8"\uf1c5 "},
    {".3ds", u8"\uf1c5 "},
    {".ttf", u8"\uf031 "},
    {".otf", u8"\uf031 "},

    // 压缩
    {".zip", u8"\uf410 "},
    {".tar", u8"\uf410 "},
    {".gz", u8"\uf410 "},
    {".7z", u8"\uf410 "},
    {".rar", u8"\uf410 "},
    {".xz", u8"\uf410 "},
    {".bz2", u8"\uf410 "},
    {".lz", u8"\uf410 "},

    // 可执行/脚本
    {".sh", u8"\uf489 "},
    {".bat", u8"\uf489 "},
    {".ps1", u8"\uf489 "},
    {".exe", u8"\uf0e7 "},
    {".msi", u8"\uf0e7 "},
    {".appimage", u8"\uf0e7 "},
    {".apk", u8"\uf17b "},
    {".ipa", u8"\uf179 "},
    {".dmg", u8"\uf179 "},
    {".deb", u8"\uf17c "},
    {".rpm", u8"\uf17c "},

    // 数据/数据库
    {".db", u8"\uf1c0 "},
    {".sqlite", u8"\uf1c0 "},
    {".sql", u8"\uf1c0 "},
    {".csv", u8"\uf1c3 "},
    {".tsv", u8"\uf1c3 "},
    {".parquet", u8"\uf1c3 "},
    {".avro", u8"\uf1c3 "},

    // 其他语言/格式
    {".html", u8"\uf13b "},
    {".htm", u8"\uf13b "},
    {".css", u8"\ue749 "},
    {".scss", u8"\ue749 "},
    {".less", u8"\ue749 "},
    {".vue", u8"\ufd42 "},
    {".svelte", u8"\ueed2 "},
    {".dart", u8"\ue798 "},
    {".rb", u8"\ue73c "},
    {".php", u8"\ue73d "},
    {".erl", u8"\ue7b1 "},
    {".tex", u8"\uf02d "},
    {".cls", u8"\uf02d "},
    {".bib", u8"\uf02d "},
    {".wsdl", u8"\ufabf "},
    {".xsd", u8"\ufabf "},
    {".proto", u8"\uf1c0 "},
    {".graphql", u8"\uf449 "},
    {".wasm", u8"\ufb41 "},
};

const std::vector<std::pair<std::string, std::string>> kFileKeywordIcons = {
    {"test", u8"\uf24e "},
    {"spec", u8"\uf24e "},
    {"mock", u8"\uf24e "},
    {"config", u8"\uf013 "},
    {"setting", u8"\uf013 "},
    {"sample", u8"\uf0c3 "},
    {"example", u8"\uf0c3 "},
    {"changelog", u8"\uf0f6 "},
    {"todo", u8"\uf0ae "},
    {"lint", u8"\uf12a "},
    {"docker", u8"\uf308 "},
    {"kube", u8"\uf1b2 "},
    {"nginx", u8"\uf233 "},
    {"apache", u8"\uf233 "},
    {"vimrc", u8"\ue62b "},
    {"zshrc", u8"\ue795 "},
    {"bashrc", u8"\ue795 "},
    {"log", u8"\uf24a "},
    {"lock", u8"\ue618 "},
};

const std::vector<std::string> kBinaryExtensions = {
    ".o", ".obj", ".a", ".so", ".dylib", ".dll", ".lib"
};

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

template <typename Table>
std::string MatchKeywordIcon(const std::string& lower_name, const Table& table) {
    for (const auto& [keyword, icon] : table) {
        if (lower_name.find(keyword) != std::string::npos) {
            return icon;
        }
    }
    return {};
}

bool HasExtension(const std::string& ext, const std::vector<std::string>& list) {
    return std::find(list.begin(), list.end(), ext) != list.end();
}

bool IsExecutable(const std::filesystem::path& path) {
    std::error_code ec;
    auto perms = std::filesystem::status(path, ec).permissions();
    if (ec) {
        return false;
    }
    return ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) ||
           ((perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none) ||
           ((perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none);
}

}  // namespace

std::string GetFolderIcon(const std::filesystem::path& path) {
    const auto name = ToLower(path.filename().string());
    if (!name.empty() && name.front() == '.') {
        return u8"\uf023 ";  // 锁图标
    }
    if (auto it = kFolderIcons.find(name); it != kFolderIcons.end()) {
        return it->second;
    }
    if (auto keyword_icon = MatchKeywordIcon(name, kFolderKeywordIcons); !keyword_icon.empty()) {
        return keyword_icon;
    }
    return DEFAULT_FOLDER_ICON;
}

std::string GetFileIcon(const std::filesystem::path& path) {
    const auto filename = ToLower(path.filename().string());
    if (auto it = kExactFileIcons.find(filename); it != kExactFileIcons.end()) {
        return it->second;
    }

    const auto ext = ToLower(path.extension().string());
    if (auto it = kExtensionIcons.find(ext); it != kExtensionIcons.end()) {
        return it->second;
    }
    if (auto keyword_icon = MatchKeywordIcon(filename, kFileKeywordIcons); !keyword_icon.empty()) {
        return keyword_icon;
    }

    if (HasExtension(ext, kBinaryExtensions)) {
        return u8"\uf471 ";
    }

    if (!path.has_extension() && IsExecutable(path)) {
        return u8"\uf144 ";  // 播放按钮图标，表示可执行
    }

    return DEFAULT_FILE_ICON;
}

std::string GetIconForPath(const std::filesystem::path& path, bool is_directory) {
    return is_directory ? GetFolderIcon(path) : GetFileIcon(path);
}

}  // namespace Icons
}  // namespace FTB


