#include "config/DefaultExtensionColors.hpp"

namespace FTB {

const std::unordered_map<std::string, std::string>& GetDefaultExtensionColors() {
    static const std::unordered_map<std::string, std::string> defaults = {

        // ============================================================
        //  Programming languages
        // ============================================================
        // Compiled / systems languages (green)
        {"c",       "#a6e3a1"}, {"cpp",     "#a6e3a1"}, {"cxx",     "#a6e3a1"},
        {"cc",      "#a6e3a1"}, {"h",       "#a6e3a1"}, {"hpp",     "#a6e3a1"},
        {"hxx",     "#a6e3a1"}, {"rs",      "#a6e3a1"}, {"rlib",    "#a6e3a1"},
        {"go",      "#a6e3a1"}, {"zig",     "#a6e3a1"}, {"nim",     "#a6e3a1"},
        {"crystal", "#a6e3a1"}, {"odin",    "#a6e3a1"}, {"v",       "#a6e3a1"},
        {"pony",    "#a6e3a1"}, {"d",       "#a6e3a1"},
        {"s",       "#a6e3a1"}, {"asm",     "#a6e3a1"}, {"S",       "#a6e3a1"},
        {"o",       "#6c7086"}, {"a",       "#6c7086"}, {"so",      "#6c7086"},
        {"class",   "#6c7086"}, {"jar",     "#6c7086"},

        // JVM languages (pink)
        {"java",    "#f5c2e7"}, {"kt",      "#f5c2e7"}, {"kts",     "#f5c2e7"},
        {"scala",   "#f5c2e7"}, {"groovy",  "#f5c2e7"}, {"clj",     "#f5c2e7"},
        {"cljs",    "#f5c2e7"},

        // .NET (blue)
        {"cs",      "#89b4fa"}, {"fs",      "#89b4fa"}, {"vb",      "#89b4fa"},
        {"xaml",    "#89b4fa"}, {"csproj",  "#6c7086"}, {"fsproj",  "#6c7086"},

        // Scripting / interpreted (blue)
        {"py",      "#89b4fa"}, {"pyw",     "#89b4fa"}, {"pyc",     "#6c7086"},
        {"pyd",     "#6c7086"}, {"pyo",     "#6c7086"},
        {"rb",      "#f38ba8"}, {"rake",    "#f38ba8"}, {"gemspec", "#f38ba8"},
        {"php",     "#89b4fa"}, {"phtml",   "#89b4fa"},
        {"pl",      "#89b4fa"}, {"pm",      "#89b4fa"}, {"t",       "#89b4fa"},
        {"lua",     "#89b4fa"}, {"luac",    "#6c7086"},
        {"r",       "#89b4fa"}, {"rmd",     "#89b4fa"}, {"rdata",   "#6c7086"},
        {"jl",      "#89b4fa"},
        {"m",       "#a6e3a1"}, {"mm",      "#a6e3a1"},
        {"swift",   "#f38ba8"}, {"dart",    "#89b4fa"},

        // JavaScript / TypeScript (pink)
        {"js",      "#f5c2e7"}, {"jsx",     "#f5c2e7"}, {"mjs",     "#f5c2e7"},
        {"cjs",     "#f5c2e7"}, {"ts",      "#f5c2e7"}, {"tsx",     "#f5c2e7"},
        {"mts",     "#f5c2e7"}, {"cts",     "#f5c2e7"},
        {"coffee",  "#f5c2e7"}, {"litcoffee", "#f5c2e7"},

        // Erlang / Elixir (purple)
        {"erl",     "#cba6f7"}, {"hrl",     "#cba6f7"},
        {"ex",      "#cba6f7"}, {"exs",     "#cba6f7"},

        // Haskell family (purple)
        {"hs",      "#cba6f7"}, {"lhs",     "#cba6f7"},
        {"elm",     "#cba6f7"}, {"purs",    "#cba6f7"}, {"idr",     "#cba6f7"},
        {"agda",    "#cba6f7"}, {"lagda",   "#cba6f7"},

        // Lisp family (purple)
        {"lisp",    "#cba6f7"}, {"lsp",     "#cba6f7"}, {"cl",      "#cba6f7"},
        {"el",      "#cba6f7"}, {"scm",     "#cba6f7"}, {"ss",      "#cba6f7"},
        {"rkt",     "#cba6f7"},

        // ML family (purple)
        {"ml",      "#cba6f7"}, {"mli",     "#cba6f7"},
        {"sml",     "#cba6f7"}, {"ocaml",   "#cba6f7"},

        // Misc languages
        {"zig",     "#a6e3a1"}, {"wgsl",    "#a6e3a1"},
        {"gleam",   "#f5c2e7"},

        // ============================================================
        //  Web technologies
        // ============================================================
        {"html",    "#f38ba8"}, {"htm",     "#f38ba8"}, {"xhtml",   "#f38ba8"},
        {"css",     "#89b4fa"}, {"scss",    "#f5c2e7"}, {"sass",    "#f5c2e7"},
        {"less",    "#f5c2e7"}, {"styl",    "#f5c2e7"},
        {"vue",     "#a6e3a1"}, {"svelte",  "#f38ba8"}, {"astro",   "#f38ba8"},
        {"hbs",     "#f38ba8"}, {"handlebars", "#f38ba8"}, {"mustache", "#f38ba8"},
        {"ejs",     "#f38ba8"}, {"jade",    "#f38ba8"}, {"pug",     "#f38ba8"},
        {"njk",     "#f38ba8"}, {"liquid",  "#f38ba8"},
        {"wasm",    "#a6e3a1"},

        // ============================================================
        //  Data / markup / config formats
        // ============================================================
        {"json",    "#f9e2af"}, {"jsonc",   "#f9e2af"}, {"json5",   "#f9e2af"},
        {"yaml",    "#f9e2af"}, {"yml",     "#f9e2af"},
        {"toml",    "#6c7086"}, {"ini",     "#6c7086"}, {"cfg",     "#6c7086"},
        {"conf",    "#6c7086"}, {"cnf",     "#6c7086"},
        {"xml",     "#89b4fa"}, {"xsd",     "#89b4fa"}, {"xslt",    "#89b4fa"},
        {"xsl",     "#89b4fa"}, {"dtd",     "#89b4fa"},
        {"svg",     "#89b4fa"},
        {"md",      "#89b4fa"}, {"markdown","#89b4fa"}, {"mdx",     "#89b4fa"},
        {"tex",     "#89b4fa"}, {"bib",     "#89b4fa"}, {"cls",     "#89b4fa"},
        {"sty",     "#89b4fa"}, {"bbl",     "#89b4fa"},
        {"rst",     "#89b4fa"}, {"adoc",    "#89b4fa"}, {"asciidoc","#89b4fa"},
        {"org",     "#89b4fa"},
        {"csv",     "#a6e3a1"}, {"tsv",     "#a6e3a1"},
        {"proto",   "#a6e3a1"}, {"graphql", "#f5c2e7"}, {"gql",     "#f5c2e7"},
        {"sql",     "#f9e2af"}, {"ddl",     "#f9e2af"}, {"dml",     "#f9e2af"},

        // ============================================================
        //  Shell / scripts
        // ============================================================
        {"sh",      "#a6e3a1"}, {"bash",    "#a6e3a1"}, {"zsh",     "#a6e3a1"},
        {"fish",    "#a6e3a1"}, {"env",     "#a6e3a1"}, {"profile", "#6c7086"},
        {"bashrc",  "#6c7086"}, {"zshrc",   "#6c7086"}, {"bash_profile", "#6c7086"},
        {"zprofile","#6c7086"}, {"aliases", "#6c7086"},
        {"awk",     "#a6e3a1"}, {"sed",     "#a6e3a1"},

        // ============================================================
        //  Build systems / CI / tooling
        // ============================================================
        {"cmake",   "#a6e3a1"}, {"cmake.in","#6c7086"},
        {"mk",      "#6c7086"}, {"makefile","#6c7086"},
        {"ninja",   "#6c7086"},
        {"bazel",   "#a6e3a1"}, {"bzl",     "#a6e3a1"},
        {"gradle",  "#a6e3a1"}, {"gradle.kts", "#a6e3a1"},
        {"scons",   "#a6e3a1"}, {"waf",     "#6c7086"},
        {"rake",    "#f38ba8"},
        {"justfile","#6c7086"},
        {"d",       "#a6e3a1"}, {"D",       "#a6e3a1"},

        // CI configuration
        {"github/workflows", "#6c7086"},
        {"gitlab-ci.yml",    "#6c7086"},
        {"drone.yml",        "#6c7086"},
        {"circleci/config",  "#6c7086"},
        {"azure-pipelines",  "#6c7086"},
        {"Jenkinsfile",      "#6c7086"},

        // ============================================================
        //  VCS / ignore files
        // ============================================================
        {"gitignore",   "#6c7086"}, {"gitattributes","#6c7086"},
        {"gitmodules",  "#6c7086"}, {"gitkeep",  "#6c7086"},
        {"svg",         "#89b4fa"},
        {"dockerignore","#6c7086"},
        {"editorconfig","#6c7086"},
        {"hgignore",    "#6c7086"},
        {"prettierignore","#6c7086"}, {"eslintignore","#6c7086"},
        {"prettierrc",  "#6c7086"}, {"eslintrc",  "#6c7086"},

        // ============================================================
        //  Images
        // ============================================================
        {"png",     "#f38ba8"}, {"jpg",     "#f38ba8"}, {"jpeg",    "#f38ba8"},
        {"jfif",    "#f38ba8"}, {"gif",     "#f38ba8"}, {"bmp",     "#f38ba8"},
        {"ico",     "#f38ba8"}, {"cur",     "#f38ba8"},
        {"webp",    "#f38ba8"}, {"avif",    "#f38ba8"}, {"heic",    "#f38ba8"},
        {"heif",    "#f38ba8"}, {"tiff",    "#f38ba8"}, {"tif",     "#f38ba8"},
        {"ppm",     "#f38ba8"}, {"pgm",     "#f38ba8"}, {"pbm",     "#f38ba8"},
        {"pnm",     "#f38ba8"},

        // ============================================================
        //  Archives / compressed
        // ============================================================
        {"zip",     "#f5c2e7"}, {"tar",     "#f5c2e7"}, {"gz",      "#f5c2e7"},
        {"gzip",    "#f5c2e7"}, {"bz2",     "#f5c2e7"}, {"bzip2",   "#f5c2e7"},
        {"xz",      "#f5c2e7"}, {"lz",      "#f5c2e7"}, {"lzma",    "#f5c2e7"},
        {"zst",     "#f5c2e7"}, {"zstd",    "#f5c2e7"},
        {"7z",      "#f5c2e7"}, {"rar",     "#f5c2e7"}, {"cbr",     "#f5c2e7"},
        {"cab",     "#f5c2e7"}, {"arj",     "#f5c2e7"},
        {"tgz",     "#f5c2e7"}, {"tbz2",    "#f5c2e7"}, {"tbz",     "#f5c2e7"},
        {"txz",     "#f5c2e7"}, {"tlz",     "#f5c2e7"}, {"tzst",    "#f5c2e7"},
        {"iso",     "#f5c2e7"}, {"vhd",     "#f5c2e7"}, {"vmdk",    "#f5c2e7"},
        {"qcow2",   "#f5c2e7"}, {"img",     "#f5c2e7"},

        // ============================================================
        //  Documents
        // ============================================================
        {"pdf",     "#f38ba8"}, {"ps",      "#f38ba8"}, {"eps",     "#f38ba8"},
        {"doc",     "#89b4fa"}, {"docx",    "#89b4fa"}, {"dotx",    "#89b4fa"},
        {"xls",     "#a6e3a1"}, {"xlsx",    "#a6e3a1"}, {"xlsm",    "#a6e3a1"},
        {"xltx",    "#a6e3a1"}, {"xltm",    "#a6e3a1"},
        {"ppt",     "#f38ba8"}, {"pptx",    "#f38ba8"}, {"pptm",    "#f38ba8"},
        {"potx",    "#f38ba8"}, {"potm",    "#f38ba8"},
        {"odt",     "#89b4fa"}, {"ods",     "#a6e3a1"}, {"odp",     "#f38ba8"},
        {"odg",     "#f38ba8"}, {"odf",     "#f38ba8"},
        {"rtf",     "#89b4fa"},
        {"epub",    "#f5c2e7"}, {"mobi",    "#f5c2e7"}, {"azw3",    "#f5c2e7"},
        {"djvu",    "#89b4fa"},
        {"pages",   "#89b4fa"}, {"numbers", "#a6e3a1"}, {"key",     "#f38ba8"},

        // ============================================================
        //  Audio
        // ============================================================
        {"mp3",     "#f5c2e7"}, {"flac",    "#f5c2e7"}, {"wav",     "#f5c2e7"},
        {"ogg",     "#f5c2e7"}, {"oga",     "#f5c2e7"},
        {"opus",    "#f5c2e7"}, {"m4a",     "#f5c2e7"}, {"m4b",     "#f5c2e7"},
        {"aac",     "#f5c2e7"}, {"wma",     "#f5c2e7"},
        {"ape",     "#f5c2e7"}, {"aiff",    "#f5c2e7"}, {"au",      "#f5c2e7"},
        {"mid",     "#f5c2e7"}, {"midi",    "#f5c2e7"},
        {"ra",      "#f5c2e7"}, {"ram",     "#f5c2e7"},

        // ============================================================
        //  Video
        // ============================================================
        {"mp4",     "#f5c2e7"}, {"mkv",     "#f5c2e7"}, {"avi",     "#f5c2e7"},
        {"mov",     "#f5c2e7"}, {"webm",    "#f5c2e7"},
        {"flv",     "#f5c2e7"}, {"wmv",     "#f5c2e7"},
        {"m4v",     "#f5c2e7"}, {"mpg",     "#f5c2e7"}, {"mpeg",    "#f5c2e7"},
        {"m2v",     "#f5c2e7"}, {"ts",      "#f5c2e7"},
        {"3gp",     "#f5c2e7"}, {"3g2",     "#f5c2e7"},
        {"ogv",     "#f5c2e7"}, {"divx",    "#f5c2e7"},
        {"vob",     "#f5c2e7"},
        {"gifv",    "#f5c2e7"},

        // Subtitle files
        {"srt",     "#89b4fa"}, {"ass",     "#89b4fa"}, {"ssa",     "#89b4fa"},
        {"vtt",     "#89b4fa"}, {"sub",     "#89b4fa"},

        // ============================================================
        //  Fonts
        // ============================================================
        {"ttf",     "#f9e2af"}, {"otf",     "#f9e2af"},
        {"woff",    "#f9e2af"}, {"woff2",   "#f9e2af"},
        {"eot",     "#f9e2af"},

        // ============================================================
        //  Database
        // ============================================================
        {"sql",     "#f9e2af"}, {"sqlite",  "#f9e2af"}, {"sqlite3", "#f9e2af"},
        {"db",      "#f9e2af"}, {"db3",     "#f9e2af"},
        {"mdb",     "#f9e2af"}, {"accdb",   "#f9e2af"},
        {"dmp",     "#6c7086"},
        {"dump",    "#6c7086"},

        // ============================================================
        //  Scientific / data analysis
        // ============================================================
        {"ipynb",   "#89b4fa"}, {"nb",      "#89b4fa"},
        {"h5",      "#89b4fa"}, {"hdf5",    "#89b4fa"}, {"he5",     "#89b4fa"},
        {"nc",      "#89b4fa"},
        {"fits",    "#89b4fa"}, {"fit",     "#89b4fa"},
        {"root",    "#89b4fa"},

        // ============================================================
        //  CAD / 3D / vector
        // ============================================================
        {"stl",     "#89b4fa"}, {"obj",     "#89b4fa"}, {"fbx",     "#89b4fa"},
        {"blend",   "#89b4fa"}, {"3ds",     "#89b4fa"},
        {"step",    "#89b4fa"}, {"stp",     "#89b4fa"}, {"iges",    "#89b4fa"},
        {"igs",     "#89b4fa"}, {"dxf",     "#89b4fa"}, {"dwg",     "#89b4fa"},
        {"glb",     "#89b4fa"}, {"gltf",    "#89b4fa"},
        {"usd",     "#89b4fa"}, {"usda",    "#89b4fa"}, {"usdc",    "#89b4fa"},

        // ============================================================
        //  Game-related
        // ============================================================
        {"unity",   "#6c7086"}, {"unitypackage", "#6c7086"},
        {"prefab",  "#6c7086"}, {"asset",   "#6c7086"}, {"meta",    "#6c7086"},
        {"fbx",     "#89b4fa"},
        {"uasset",  "#6c7086"}, {"umap",    "#6c7086"},
        {"godot",   "#89b4fa"}, {"tscn",    "#89b4fa"}, {"tres",    "#89b4fa"},
        {"gd",      "#89b4fa"},
        {"rpmap",   "#6c7086"}, {"rprefab", "#6c7086"},

        // ============================================================
        //  Temporary / backup / misc
        // ============================================================
        {"tmp",     "#6c7086"}, {"temp",    "#6c7086"},
        {"swp",     "#6c7086"}, {"swo",     "#6c7086"}, {"swn",     "#6c7086"},
        {"bak",     "#6c7086"}, {"old",     "#6c7086"}, {"orig",    "#6c7086"},
        {"backup",  "#6c7086"}, {"~",       "#6c7086"},
        {"part",    "#6c7086"}, {"crdownload","#6c7086"},
        {"lock",    "#6c7086"}, {"log",     "#6c7086"},
        {"out",     "#6c7086"}, {"err",     "#6c7086"},
        {"core",    "#6c7086"},
        {"dump",    "#6c7086"},

        // ============================================================
        //  OS / environment files
        // ============================================================
        {"desktop", "#6c7086"}, {"service", "#6c7086"},
        {"timer",   "#6c7086"}, {"mount",   "#6c7086"},
        {"socket",  "#6c7086"}, {"path",    "#6c7086"},
        {"target",  "#6c7086"},
        {"pacnew",  "#6c7086"}, {"pacsave", "#6c7086"},

        // ============================================================
        //  Patch / diff
        // ============================================================
        {"patch",   "#a6e3a1"}, {"diff",    "#a6e3a1"},

        // ============================================================
        //  Package files
        // ============================================================
        {"deb",     "#f38ba8"}, {"rpm",     "#f38ba8"},
        {"pkg",     "#f38ba8"}, {"apk",     "#f38ba8"},
        {"AppImage","#f38ba8"}, {"flatpak", "#f38ba8"},
        {"snap",    "#f38ba8"},
        {"dmg",     "#f38ba8"},
        {"exe",     "#a6e3a1"}, {"msi",     "#a6e3a1"},
        {"dll",     "#6c7086"}, {"so",      "#6c7086"}, {"dylib",   "#6c7086"},

        // ============================================================
        //  Key / certificate / security
        // ============================================================
        {"pem",     "#f9e2af"}, {"crt",     "#f9e2af"}, {"cert",    "#f9e2af"},
        {"key",     "#f9e2af"}, {"pub",     "#f9e2af"}, {"priv",    "#f9e2af"},
        {"p12",     "#f9e2af"}, {"pfx",     "#f9e2af"}, {"jks",     "#f9e2af"},
        {"gpg",     "#f9e2af"}, {"asc",     "#f9e2af"},

        // ============================================================
        //  Container / orchestration
        // ============================================================
        {"dockerfile", "#89b4fa"},
        {"yml",     "#f9e2af"}, {"yaml",    "#f9e2af"},

    };
    return defaults;
}

} // namespace FTB
