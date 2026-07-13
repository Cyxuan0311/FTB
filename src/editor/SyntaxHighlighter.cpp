#include "editor/SyntaxHighlighter.hpp"
#include "editor/SyntaxHighlighterTreeSitter.hpp"
#include "config/ThemeManager.hpp"
#include "utils/PerfLogger.hpp"
#include "utils/UnicodeUtil.hpp"
#include <algorithm>
#include <cctype>
#include <functional>

using namespace ftxui;

namespace FTB {
namespace Editor {

// ---- 主题感知颜色辅助 ----
inline Color TC(const std::string& name) {
    return FTB::ThemeManager::GetInstance()->GetThemeColor(name);
}

// ---- 构造/析构 ----

SyntaxHighlighter::SyntaxHighlighter() : current_language_(Language::NONE) {
    InitializePatterns();
    ts_highlighter_ = std::make_unique<SyntaxHighlighterTreeSitter>();
    if (SyntaxHighlighterTreeSitter::IsAvailable()) {
        backend_ = HighlightBackend::TREE_SITTER;
    }
}

SyntaxHighlighter::~SyntaxHighlighter() {}

void SyntaxHighlighter::SetLanguage(Language lang) {
    Language old = current_language_;
    if (current_language_ != lang) {
        current_language_ = lang;
        ResetMultiLineState();
        token_cache_.clear();
        ts_highlighter_->SetLanguage(lang);
    }
}

void SyntaxHighlighter::SetBackend(HighlightBackend backend) {
    backend_ = backend;
    if (backend_ == HighlightBackend::TREE_SITTER && SyntaxHighlighterTreeSitter::IsAvailable()) {
        ts_highlighter_->SetLanguage(current_language_);
        ts_highlighter_->Reset();
    }
}

void SyntaxHighlighter::ResetMultiLineState() {
    in_multiline_comment_ = false;
    ts_highlighter_->Reset();
}

// ---- 语言关键词/类型初始化 ----

void SyntaxHighlighter::InitializePatterns() {
    // ===== C/C++ =====
    keyword_patterns_[Language::C] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
        "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return",
        "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile", "while", "asm"
    };
    keyword_patterns_[Language::CPP] = {
        // C 基础
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
        "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return",
        "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile", "while", "asm",
        // C++ 核心
        "bool", "catch", "class", "const_cast", "delete", "dynamic_cast", "explicit", "export",
        "friend", "inline", "mutable", "namespace", "new", "operator", "private", "protected",
        "public", "reinterpret_cast", "static_cast", "template", "this", "throw", "try", "typeid",
        "typename", "using", "virtual", "override", "final",
        // C++11/14/17/20/23
        "alignas", "alignof", "char16_t", "char32_t", "char8_t", "constexpr", "consteval",
        "constinit", "decltype", "noexcept", "nullptr", "static_assert", "thread_local",
        "wchar_t", "co_await", "co_return", "co_yield", "concept", "requires", "module", "import",
        "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq",
        "true", "false"
    };
    type_patterns_[Language::C] = {
        "int", "char", "float", "double", "void", "bool", "wchar_t",
        "size_t", "ptrdiff_t", "nullptr_t", "FILE", "ssize_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t"
    };
    type_patterns_[Language::CPP] = {
        "int", "char", "float", "double", "void", "bool", "wchar_t",
        "size_t", "ptrdiff_t", "nullptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "string", "vector", "map", "set", "list", "deque", "queue", "stack",
        "unordered_map", "unordered_set", "pair", "tuple", "optional", "variant", "any",
        "shared_ptr", "unique_ptr", "weak_ptr", "string_view", "span", "array",
        "multiset", "multimap", "bitset", "valarray", "complex", "regex",
        "chrono", "mutex", "atomic", "thread", "future", "promise",
        "exception", "runtime_error", "logic_error",
        "iterator", "const_iterator", "reverse_iterator",
        "size_type", "value_type", "difference_type"
    };

    // ===== Go =====
    keyword_patterns_[Language::GO] = {
        "break", "case", "chan", "const", "continue", "default", "defer", "else", "fallthrough",
        "for", "func", "go", "goto", "if", "import", "interface", "map", "package", "range",
        "return", "select", "struct", "switch", "type", "var",
        "true", "false", "nil", "iota", "append", "cap", "close", "complex", "copy", "delete",
        "imag", "len", "make", "new", "panic", "print", "println", "real", "recover"
    };
    type_patterns_[Language::GO] = {
        "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
        "uintptr", "float32", "float64", "complex64", "complex128", "byte", "rune", "string",
        "bool", "error", "any", "comparable"
    };

    // ===== Python =====
    keyword_patterns_[Language::PYTHON] = {
        "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del",
        "elif", "else", "except", "False", "finally", "for", "from", "global", "if", "import",
        "in", "is", "lambda", "None", "nonlocal", "not", "or", "pass", "raise", "return",
        "True", "try", "while", "with", "yield", "self"
    };
    type_patterns_[Language::PYTHON] = {
        "int", "float", "str", "bool", "list", "dict", "tuple", "set", "bytes", "bytearray",
        "object", "type", "range", "frozenset", "complex", "memoryview", "ellipsis"
    };

    // ===== JavaScript =====
    keyword_patterns_[Language::JAVASCRIPT] = {
        "async", "await", "break", "case", "catch", "class", "const", "continue", "debugger",
        "default", "delete", "do", "else", "enum", "export", "extends", "false", "finally",
        "for", "function", "if", "implements", "import", "in", "instanceof", "interface", "let",
        "new", "null", "package", "private", "protected", "public", "return", "static", "super",
        "switch", "this", "throw", "true", "try", "typeof", "var", "void", "while", "with", "yield"
    };
    type_patterns_[Language::JAVASCRIPT] = {
        "Array", "Boolean", "Date", "Error", "Function", "JSON", "Math", "Number", "Object",
        "Promise", "RegExp", "String", "Symbol", "Map", "Set", "WeakMap", "WeakSet",
        "ArrayBuffer", "DataView", "Int8Array", "Uint8Array", "Float32Array", "Float64Array",
        "console", "window", "document", "process"
    };

    // ===== TypeScript =====
    keyword_patterns_[Language::TYPESCRIPT] = keyword_patterns_[Language::JAVASCRIPT];
    keyword_patterns_[Language::TYPESCRIPT].insert({
        "interface", "type", "namespace", "module", "declare", "abstract", "readonly", "as", "satisfies",
        "keyof", "infer", "extends"
    });
    type_patterns_[Language::TYPESCRIPT] = type_patterns_[Language::JAVASCRIPT];
    type_patterns_[Language::TYPESCRIPT].insert({
        "Promise", "Iterable", "Iterator", "Generator", "AsyncIterator",
        "Record", "Partial", "Required", "Readonly", "Pick", "Omit", "Exclude", "Extract",
        "NonNullable", "ReturnType", "InstanceType", "Parameters", "ConstructorParameters"
    });

    // ===== Rust =====
    keyword_patterns_[Language::RUST] = {
        "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else", "enum",
        "extern", "false", "fn", "for", "if", "impl", "in", "let", "loop", "match", "mod",
        "move", "mut", "pub", "ref", "return", "self", "Self", "static", "struct", "super",
        "trait", "true", "type", "unsafe", "use", "where", "while", "abstract", "become",
        "box", "do", "final", "macro", "override", "priv", "typeof", "unsized", "virtual",
        "yield", "try", "gen"
    };
    type_patterns_[Language::RUST] = {
        "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize",
        "f32", "f64", "bool", "char", "str", "String", "Option", "Result", "Vec", "Box", "Rc", "Arc",
        "RefCell", "Mutex", "RwLock", "Cell", "HashMap", "HashSet", "BTreeMap", "BTreeSet",
        "Iterator", "IntoIterator", "FromIterator", "Clone", "Copy", "Send", "Sync",
        "Debug", "Display", "Default", "Drop", "Fn", "FnMut", "FnOnce", "Sized", "Error"
    };

    // ===== Java =====
    keyword_patterns_[Language::JAVA] = {
        "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class",
        "const", "continue", "default", "do", "double", "else", "enum", "extends", "final",
        "finally", "float", "for", "goto", "if", "implements", "import", "instanceof", "int",
        "interface", "long", "native", "new", "package", "private", "protected", "public",
        "return", "short", "static", "strictfp", "super", "switch", "synchronized", "this",
        "throw", "throws", "transient", "try", "void", "volatile", "while",
        "true", "false", "null", "record", "sealed", "permits", "var", "yield"
    };
    type_patterns_[Language::JAVA] = {
        "Object", "String", "Integer", "Long", "Short", "Byte", "Float", "Double", "Boolean",
        "Character", "Void", "Number", "Class", "Enum", "Exception", "Error", "Throwable",
        "Runnable", "Comparable", "Iterable", "Collection", "List", "Set", "Map",
        "ArrayList", "LinkedList", "HashMap", "HashSet", "Optional", "Stream",
        "StringBuilder", "StringBuffer", "System", "Math", "Arrays", "Collections"
    };

    // ===== C# =====
    keyword_patterns_[Language::CSHARP] = {
        "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked",
        "class", "const", "continue", "decimal", "default", "delegate", "do", "double", "else",
        "enum", "event", "explicit", "extern", "false", "finally", "fixed", "float", "for",
        "foreach", "goto", "if", "implicit", "in", "int", "interface", "internal", "is", "lock",
        "long", "namespace", "new", "null", "object", "operator", "out", "override", "params",
        "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short",
        "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true",
        "try", "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual",
        "void", "volatile", "while", "yield", "var", "dynamic", "async", "await", "nameof", "when",
        "record", "init", "with", "required", "file", "scoped", "global", "nint", "nuint"
    };
    type_patterns_[Language::CSHARP] = {
        "bool", "byte", "sbyte", "char", "decimal", "double", "float", "int", "uint", "long",
        "ulong", "short", "ushort", "object", "string", "dynamic", "void", "var",
        "DateTime", "TimeSpan", "Guid", "Array", "List", "Dictionary", "HashSet", "Queue", "Stack",
        "Tuple", "ValueTuple", "Task", "IEnumerable", "ICollection", "IList", "IDictionary",
        "Exception", "System", "Console", "Math", "StringBuilder", "Regex"
    };

    // ===== Ruby =====
    keyword_patterns_[Language::RUBY] = {
        "BEGIN", "END", "alias", "and", "begin", "break", "case", "class", "def", "defined?",
        "do", "else", "elsif", "end", "ensure", "false", "for", "if", "in", "module", "next",
        "nil", "not", "or", "redo", "rescue", "retry", "return", "self", "super", "then", "true",
        "undef", "unless", "until", "when", "while", "yield"
    };
    type_patterns_[Language::RUBY] = {
        "Array", "Hash", "String", "Integer", "Float", "Symbol", "Regexp", "Range", "Time",
        "Date", "DateTime", "File", "Dir", "IO", "Exception"
    };

    // ===== PHP =====
    keyword_patterns_[Language::PHP] = {
        "abstract", "and", "array", "as", "break", "callable", "case", "catch", "class", "clone",
        "const", "continue", "declare", "default", "die", "do", "echo", "else", "elseif", "empty",
        "enddeclare", "endfor", "endforeach", "endif", "endswitch", "endwhile", "enum", "eval",
        "exit", "extends", "final", "finally", "fn", "for", "foreach", "function", "global",
        "goto", "if", "implements", "include", "include_once", "instanceof", "insteadof",
        "interface", "isset", "list", "match", "namespace", "new", "or", "print", "private",
        "protected", "public", "readonly", "require", "require_once", "return", "static",
        "switch", "throw", "trait", "try", "unset", "use", "var", "while", "xor", "yield",
        "true", "false", "null"
    };
    type_patterns_[Language::PHP] = {
        "int", "float", "string", "bool", "array", "object", "callable", "iterable", "void",
        "mixed", "never", "stdClass", "Exception", "Error", "Throwable", "DateTime",
        "PDO", "Closure", "Generator"
    };

    // ===== Swift =====
    keyword_patterns_[Language::SWIFT] = {
        "associatedtype", "class", "deinit", "enum", "extension", "fileprivate", "func", "import",
        "init", "inout", "internal", "let", "open", "operator", "private", "protocol", "public",
        "rethrows", "static", "struct", "subscript", "typealias", "var", "break", "case",
        "continue", "default", "defer", "do", "else", "fallthrough", "for", "guard", "if", "in",
        "repeat", "return", "switch", "where", "while", "as", "Any", "catch", "false", "is",
        "nil", "rethrows", "super", "self", "Self", "throw", "throws", "true", "try"
    };
    type_patterns_[Language::SWIFT] = {
        "Int", "Int8", "Int16", "Int32", "Int64", "UInt", "UInt8", "UInt16", "UInt32", "UInt64",
        "Float", "Double", "Bool", "String", "Character", "Array", "Dictionary", "Set",
        "Optional", "Any", "AnyObject", "AnyClass", "Void", "Never", "Error", "Result", "URL", "Data"
    };

    // ===== Kotlin =====
    keyword_patterns_[Language::KOTLIN] = {
        "as", "as?", "break", "class", "continue", "do", "else", "false", "for", "fun", "if",
        "in", "!in", "interface", "is", "!is", "null", "object", "package", "return", "super",
        "this", "throw", "true", "try", "typealias", "typeof", "val", "var", "when", "while",
        "by", "catch", "constructor", "delegate", "dynamic", "field", "file", "finally", "get",
        "import", "init", "param", "property", "receiver", "set", "setparam", "where",
        "actual", "abstract", "annotation", "companion", "const", "crossinline", "data", "enum",
        "expect", "external", "final", "infix", "inline", "inner", "internal", "lateinit",
        "noinline", "open", "operator", "out", "override", "private", "protected", "public",
        "reified", "sealed", "suspend", "tailrec", "vararg", "it"
    };
    type_patterns_[Language::KOTLIN] = {
        "Any", "Unit", "Nothing", "Int", "Long", "Short", "Byte", "Float", "Double", "Boolean",
        "Char", "String", "Array", "List", "MutableList", "Set", "MutableSet", "Map", "MutableMap",
        "Pair", "Triple", "Collection", "Sequence", "Exception"
    };

    // ===== Scala =====
    keyword_patterns_[Language::SCALA] = {
        "abstract", "case", "catch", "class", "def", "do", "else", "extends", "false", "final",
        "finally", "for", "forSome", "if", "implicit", "import", "lazy", "macro", "match", "new",
        "null", "object", "override", "package", "private", "protected", "return", "sealed",
        "super", "this", "throw", "trait", "try", "true", "type", "val", "var", "while", "with", "yield"
    };
    type_patterns_[Language::SCALA] = {
        "Any", "AnyRef", "AnyVal", "Nothing", "Null", "Unit", "Boolean", "Byte", "Char", "Short",
        "Int", "Long", "Float", "Double", "String", "List", "Array", "Seq", "Set", "Map",
        "Option", "Either", "Try", "Future", "Promise", "Iterable", "Iterator", "Vector"
    };

    // ===== Lua =====
    keyword_patterns_[Language::LUA] = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if",
        "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
    };
    type_patterns_[Language::LUA] = {
        "number", "string", "boolean", "table", "function", "thread", "userdata",
        "type", "pairs", "ipairs", "next", "tostring", "tonumber", "print", "require",
        "package", "io", "os", "math", "string", "table", "coroutine", "debug"
    };

    // ===== Shell/Bash =====
    keyword_patterns_[Language::SHELL] = keyword_patterns_[Language::BASH] = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while", "do", "done", "in",
        "function", "return", "exit", "break", "continue", "echo", "export", "source", "cd",
        "pwd", "ls", "cat", "grep", "sed", "awk", "find", "mkdir", "rm", "cp", "mv", "chmod",
        "chown", "sudo", "apt", "yum", "dnf", "pacman", "brew", "pip", "npm", "cargo", "go",
        "local", "readonly", "declare", "typeset", "unset", "shift", "eval", "exec", "trap",
        "wait", "read", "printf", "test", "true", "false"
    };

    // ===== SQL =====
    keyword_patterns_[Language::SQL] = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE", "DROP", "ALTER",
        "TABLE", "INDEX", "VIEW", "DATABASE", "SCHEMA", "COLUMN", "CONSTRAINT", "PRIMARY",
        "FOREIGN", "UNIQUE", "NOT", "NULL", "DEFAULT", "JOIN", "INNER", "LEFT", "RIGHT", "FULL",
        "OUTER", "ON", "GROUP", "BY", "HAVING", "ORDER", "ASC", "DESC", "LIMIT", "OFFSET",
        "UNION", "ALL", "DISTINCT", "AS", "AND", "OR", "IN", "EXISTS", "BETWEEN", "LIKE", "IS",
        "BEGIN", "COMMIT", "ROLLBACK", "TRANSACTION", "CASE", "WHEN", "THEN", "ELSE", "END",
        "IF", "SET", "INTO", "VALUES", "INT"
    };

    // ===== HTML =====
    keyword_patterns_[Language::HTML] = {
        "html", "head", "body", "div", "span", "p", "a", "img", "br", "hr", "table", "tr", "td",
        "th", "ul", "ol", "li", "h1", "h2", "h3", "h4", "h5", "h6", "form", "input", "button",
        "select", "option", "textarea", "label", "script", "style", "link", "meta", "title",
        "DOCTYPE", "class", "id", "name", "type", "value", "href", "src", "alt"
    };

    // ===== CSS =====
    keyword_patterns_[Language::CSS] = {
        "import", "media", "keyframes", "animation", "transition", "transform", "background",
        "color", "font", "margin", "padding", "border", "width", "height", "display", "position",
        "float", "clear", "overflow", "z-index", "opacity", "visibility", "cursor", "flex",
        "grid", "box", "shadow", "radius", "gradient", "important"
    };

    // ===== XML =====
    keyword_patterns_[Language::XML] = {
        "DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY", "NOTATION", "CDATA", "xml", "version",
        "encoding", "standalone"
    };

    // ===== JSON =====
    keyword_patterns_[Language::JSON] = {"true", "false", "null"};

    // ===== YAML =====
    keyword_patterns_[Language::YAML] = {"true", "false", "null", "yes", "no", "on", "off"};

    // ===== TOML =====
    keyword_patterns_[Language::TOML] = {"true", "false", "null", "nan", "inf"};

    // ===== Markdown =====
    // Markdown 没有传统关键字，但可以标记一些特殊词
    keyword_patterns_[Language::MARKDOWN] = {};

    // ===== CMake =====
    keyword_patterns_[Language::CMAKE] = {
        "if", "else", "elseif", "endif", "foreach", "endforeach", "while", "endwhile",
        "function", "endfunction", "macro", "endmacro", "break", "continue", "return",
        "cmake_minimum_required", "project", "add_executable", "add_library",
        "target_link_libraries", "include", "include_directories", "find_package",
        "set", "unset", "list", "string", "math", "file", "message", "option",
        "add_subdirectory", "install", "enable_testing", "add_test", "set_property",
        "get_property", "target_compile_options", "target_include_directories"
    };
    type_patterns_[Language::CMAKE] = {
        "CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER", "CMAKE_BUILD_TYPE",
        "CMAKE_SOURCE_DIR", "CMAKE_BINARY_DIR", "CMAKE_CURRENT_SOURCE_DIR",
        "CMAKE_CURRENT_BINARY_DIR", "CMAKE_VERSION", "PROJECT_NAME", "PROJECT_VERSION"
    };

    // ===== Makefile =====
    keyword_patterns_[Language::MAKEFILE] = {
        "include", "define", "endef", "ifdef", "ifndef", "ifeq", "ifneq", "else", "endif",
        "override", "export", "unexport", "private", "vpath", "all", "clean", "install",
        ".PHONY", ".SUFFIXES", ".DEFAULT", ".PRECIOUS"
    };

    // ===== Dockerfile =====
    keyword_patterns_[Language::DOCKERFILE] = {
        "FROM", "MAINTAINER", "RUN", "CMD", "LABEL", "EXPOSE", "ENV", "ADD", "COPY",
        "ENTRYPOINT", "VOLUME", "USER", "WORKDIR", "ARG", "ONBUILD", "STOPSIGNAL",
        "HEALTHCHECK", "SHELL", "AS"
    };

    // ===== Vim =====
    keyword_patterns_[Language::VIMSCRIPT] = {
        "if", "else", "elseif", "endif", "for", "in", "endfor", "while", "endwhile",
        "try", "catch", "finally", "endtry", "throw", "function", "endfunction", "return",
        "break", "continue", "let", "unlet", "const", "call", "execute", "eval", "echo",
        "echomsg", "echoerr", "normal", "silent", "command", "autocmd", "augroup",
        "syntax", "highlight", "set", "setlocal", "map", "nmap", "vmap", "imap",
        "nnoremap", "vnoremap", "inoremap"
    };

    // ===== Zig =====
    keyword_patterns_[Language::ZIG] = {
        "addrspace", "align", "and", "anyframe", "anytype", "asm", "async", "await", "break",
        "catch", "comptime", "const", "continue", "defer", "else", "enum", "errdefer", "error",
        "export", "extern", "fn", "for", "if", "inline", "noalias", "nosuspend", "orelse", "or",
        "packed", "pub", "resume", "return", "linksection", "struct", "suspend", "switch", "test",
        "threadlocal", "try", "union", "unreachable", "usingnamespace", "var", "volatile", "while"
    };
    type_patterns_[Language::ZIG] = {
        "void", "bool", "noreturn", "type", "anyerror", "u8", "i8", "u16", "i16", "u32", "i32",
        "u64", "i64", "u128", "i128", "usize", "isize", "f16", "f32", "f64", "f80", "f128",
        "null", "undefined"
    };

    // ===== Elixir =====
    keyword_patterns_[Language::ELIXIR] = {
        "after", "and", "catch", "do", "else", "end", "fn", "in", "not", "or", "rescue", "when",
        "case", "cond", "for", "if", "unless", "try", "with", "receive", "raise", "throw",
        "def", "defp", "defmodule", "defstruct", "defprotocol", "defimpl", "defmacro",
        "defmacrop", "defguard", "defguardp", "defcallback", "defoverridable", "use", "import",
        "alias", "require", "quote", "unquote", "super", "true", "false", "nil"
    };

    // ===== Perl =====
    keyword_patterns_[Language::PERL] = {
        "if", "unless", "while", "until", "for", "foreach", "when", "given", "default", "break",
        "next", "last", "redo", "continue", "return", "sub", "my", "our", "local", "state",
        "use", "no", "package", "require", "BEGIN", "END", "eval", "do", "strict", "warnings"
    };

    // ===== R =====
    keyword_patterns_[Language::R] = {
        "if", "else", "repeat", "while", "function", "for", "in", "next", "break",
        "TRUE", "FALSE", "NULL", "Inf", "NaN", "NA", "library", "require", "source",
        "attach", "detach", "ls", "rm", "save", "load", "data", "print", "cat", "paste"
    };
    type_patterns_[Language::R] = {
        "numeric", "integer", "double", "complex", "character", "logical", "factor",
        "matrix", "array", "data.frame", "list", "vector", "Date", "POSIXct", "POSIXlt"
    };

    // ===== Haskell =====
    keyword_patterns_[Language::HASKELL] = {
        "data", "type", "newtype", "class", "instance", "where", "let", "in", "if", "then",
        "else", "case", "of", "do", "deriving", "import", "module", "qualified", "as", "hiding",
        "infix", "infixl", "infixr", "default", "foreign", "forall", "mdo", "rec", "proc"
    };
    type_patterns_[Language::HASKELL] = {
        "Int", "Integer", "Float", "Double", "Bool", "Char", "String", "Maybe", "Either", "IO",
        "Ordering", "Eq", "Ord", "Show", "Read", "Enum", "Bounded", "Num", "Functor",
        "Applicative", "Monad", "Monoid", "Foldable", "Traversable"
    };

    // ===== Fortran =====
    keyword_patterns_[Language::FORTRAN] = {
        "program", "end", "subroutine", "function", "module", "use", "contains", "implicit",
        "none", "integer", "real", "double", "precision", "complex", "logical", "character",
        "allocatable", "dimension", "parameter", "data", "if", "then", "else", "elseif", "endif",
        "select", "case", "default", "do", "while", "enddo", "continue", "cycle", "exit",
        "goto", "return", "call", "intent", "in", "out", "inout", "optional", "save",
        "public", "private", "interface", "abstract", "type", "procedure", "allocate",
        "deallocate", "nullify", "associate"
    };

    // ===== Nim =====
    keyword_patterns_[Language::NIM] = {
        "addr", "and", "as", "asm", "atomic", "bind", "block", "break", "case", "cast",
        "concept", "const", "continue", "converter", "defer", "discard", "distinct", "div",
        "do", "elif", "else", "end", "enum", "except", "export", "finally", "for", "from",
        "func", "if", "import", "in", "include", "interface", "is", "isnot", "iterator", "let",
        "macro", "method", "mixin", "mod", "nil", "not", "notin", "object", "of", "or", "out",
        "proc", "ptr", "raise", "ref", "return", "shl", "shr", "static", "template", "try",
        "tuple", "type", "using", "var", "when", "while", "with", "without", "xor", "yield"
    };
    type_patterns_[Language::NIM] = {
        "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
        "float", "float32", "float64", "bool", "char", "string", "cstring", "pointer", "byte",
        "seq", "array", "openarray", "varargs", "set", "object", "tuple", "ref", "ptr", "proc",
        "iterator", "distinct", "void"
    };

    // ===== Proto =====
    keyword_patterns_[Language::PROTO] = {
        "syntax", "import", "option", "package", "message", "enum", "service", "rpc", "returns",
        "optional", "repeated", "map", "oneof", "reserved", "extend", "to", "stream"
    };
    type_patterns_[Language::PROTO] = {
        "int32", "int64", "uint32", "uint64", "sint32", "sint64", "fixed32", "fixed64",
        "sfixed32", "sfixed64", "bool", "string", "bytes", "float", "double"
    };

    // ===== Meson =====
    keyword_patterns_[Language::MESON] = {
        "project", "meson", "subdir", "executable", "library", "shared_library", "static_library",
        "dependency", "find_program", "find_library", "compiler", "subproject", "get_option",
        "option", "files", "include_directories", "declare_dependency", "summary", "warning",
        "error", "message", "assert", "if", "else", "elif", "endif", "foreach", "endforeach",
        "break", "continue", "return", "and", "or", "not", "true", "false", "import",
        "run_command", "configure_file", "install_data", "install_headers", "install_man"
    };

    // ===== PowerShell =====
    keyword_patterns_[Language::POWERSHELL] = {
        "begin", "break", "catch", "class", "continue", "data", "do", "dynamicparam", "else",
        "elseif", "end", "enum", "exit", "filter", "finally", "for", "foreach", "from",
        "function", "if", "in", "param", "process", "return", "switch", "throw", "trap", "try",
        "until", "using", "var", "while", "workflow", "configuration",
        "write-host", "write-output", "write-error", "get-content", "set-content",
        "get-childitem", "test-path", "new-object", "select-object", "where-object"
    };

    // ===== Dart =====
    keyword_patterns_[Language::DART] = {
        "abstract", "as", "assert", "async", "await", "break", "case", "catch", "class", "const",
        "continue", "covariant", "default", "deferred", "do", "dynamic", "else", "enum", "export",
        "extends", "extension", "external", "factory", "false", "final", "finally", "for", "Function",
        "get", "hide", "if", "implements", "import", "in", "interface", "is", "late", "library",
        "mixin", "new", "null", "on", "operator", "part", "required", "rethrow", "return", "set",
        "show", "static", "super", "switch", "sync", "this", "throw", "true", "try", "typedef",
        "var", "void", "while", "with", "yield"
    };
    type_patterns_[Language::DART] = {
        "int", "double", "num", "bool", "String", "dynamic", "Never", "Null", "Object", "void",
        "Record", "Future", "Stream", "Iterable", "List", "Set", "Map", "Comparable", "Pattern",
        "RegExp", "Symbol", "DateTime", "Duration", "Error", "Exception", "StackTrace",
        "Type", "Uri", "Never", "num"
    };

    // ===== Julia =====
    keyword_patterns_[Language::JULIA] = {
        "abstract", "baremodule", "begin", "break", "catch", "ccall", "const", "continue", "do",
        "else", "elseif", "end", "enum", "export", "finally", "for", "function", "global", "if",
        "import", "in", "let", "local", "macro", "module", "mutable", "primitive", "public", "quote",
        "return", "struct", "try", "type", "using", "where", "while",
        "true", "false", "nothing", "missing"
    };
    type_patterns_[Language::JULIA] = {
        "Int8", "Int16", "Int32", "Int64", "Int128", "UInt8", "UInt16", "UInt32", "UInt64", "UInt128",
        "Float16", "Float32", "Float64", "BigInt", "BigFloat", "String", "Char", "Bool", "Symbol",
        "Array", "Vector", "Matrix", "Tuple", "NamedTuple", "Dict", "Set", "Pair", "Complex",
        "Rational", "Regex", "Nothing", "Missing", "Any", "Union", "TypeVar", "AbstractArray",
        "AbstractString", "Number", "Real", "AbstractFloat", "Integer", "Signed", "Unsigned"
    };

    // ===== Erlang =====
    keyword_patterns_[Language::ERLANG] = {
        "after", "and", "andalso", "band", "begin", "bnot", "bor", "bsl", "bsr", "bxor", "case",
        "catch", "cond", "div", "end", "fun", "if", "let", "not", "of", "or", "orelse", "query",
        "receive", "rem", "try", "when", "xor",
        "module", "export", "import", "record", "define", "include", "include_lib", "behaviour",
        "spec", "callback", "type", "opaque",
        "true", "false", "ok", "error", "undefined"
    };
    type_patterns_[Language::ERLANG] = {
        "list", "tuple", "atom", "string", "integer", "float", "binary", "map", "pid", "port",
        "reference", "fun", "boolean", "char", "term", "any", "maybe_improper_list",
        "nonempty_list", "nonempty_string"
    };

    // ===== Clojure =====
    keyword_patterns_[Language::CLOJURE] = {
        "def", "defn", "defmacro", "defprotocol", "defrecord", "deftype", "defmulti", "defmethod",
        "let", "letfn", "do", "if", "if-not", "when", "when-not", "when-let", "when-first",
        "cond", "condp", "case", "for", "loop", "recur", "fn", "throw", "try", "catch", "finally",
        "monitor-enter", "monitor-exit", "import", "ns", "in-ns", "refer", "alias", "use", "require",
        "binding", "with-open", "with-local-vars", "doseq", "dotimes", "while", "comment",
        "true", "false", "nil"
    };
    type_patterns_[Language::CLOJURE] = {
        "Integer", "Long", "Float", "Double", "String", "Boolean", "List", "Vector", "HashMap",
        "Set", "Ratio", "Symbol", "Keyword", "Atom", "Ref", "Agent", "Delay", "Promise",
        "Future", "Var", "Namespace", "MultiFn", "Protocol", "Type", "Record"
    };

    // ===== F# =====
    keyword_patterns_[Language::FSHARP] = {
        "abstract", "and", "as", "assert", "async", "base", "begin", "class", "default", "delegate",
        "do", "done", "downcast", "downto", "elif", "else", "end", "exception", "extern", "false",
        "finally", "fixed", "for", "fun", "function", "global", "if", "in", "inherit", "inline",
        "interface", "internal", "lazy", "let", "match", "member", "module", "mutable", "namespace",
        "new", "not", "null", "of", "open", "or", "override", "private", "public", "rec", "return",
        "sig", "static", "struct", "then", "to", "true", "try", "type", "upcast", "use", "val",
        "void", "when", "while", "with", "yield"
    };
    type_patterns_[Language::FSHARP] = {
        "int", "float", "bool", "char", "string", "unit", "byte", "decimal", "double", "single",
        "int16", "int32", "int64", "uint16", "uint32", "uint64", "nativeint", "unativeint",
        "bigint", "obj", "exn", "seq", "list", "array", "option", "Map", "Set", "Async", "Task",
        "Result", "Choice", "ValueOption"
    };
}

// ---- 语言检测 ----

Language SyntaxHighlighter::DetectLanguage(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // 精确文件名匹配
    if (lower == "cmakelists.txt" || lower == "cmakecache.txt") return Language::CMAKE;
    if (lower == "dockerfile" || lower == "containerfile") return Language::DOCKERFILE;
    if (lower == "makefile" || lower == "gnumakefile" || lower == "makefile.am" || lower == "makefile.in") return Language::MAKEFILE;
    if (lower == "justfile") return Language::MAKEFILE;
    if (lower == "meson.build" || lower == "meson_options.txt") return Language::MESON;
    if (lower == "vagrantfile") return Language::RUBY;
    if (lower == "rakefile" || lower == "gemfile") return Language::RUBY;
    if (lower == "gemfile.lock") return Language::RUBY;
    if (lower == "jenkinsfile") return Language::SHELL;
    if (lower == ".env" || lower.rfind(".env.", 0) == 0) return Language::SHELL;
    if (lower == "gradlew" || lower == "gradlew.bat") return Language::SHELL;
    if (lower == "go.mod" || lower == "go.sum" || lower == "go.work") return Language::GO;
    if (lower == "yarn.lock") return Language::YAML;
    if (lower == ".clang-format" || lower == ".clang-tidy" || lower == ".clangd") return Language::YAML;
    if (lower.rfind("dockerfile.", 0) == 0) return Language::DOCKERFILE; // Dockerfile.alpine, etc.
    if (lower == "rebar.config" || lower == "rebar.lock") return Language::ERLANG;
    if (lower == ".gitignore" || lower == ".dockerignore" || lower == ".npmignore") return Language::NONE;

    // 扩展名匹配
    auto dot = lower.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = lower.substr(dot);
        if (ext == ".c" || ext == ".h") return Language::C;
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".hpp" ||
            ext == ".hxx" || ext == ".hh" || ext == ".ipp" || ext == ".tcc") return Language::CPP;
        if (ext == ".go") return Language::GO;
        if (ext == ".py" || ext == ".pyw" || ext == ".pyi") return Language::PYTHON;
        if (ext == ".js" || ext == ".mjs" || ext == ".cjs") return Language::JAVASCRIPT;
        if (ext == ".ts" || ext == ".tsx") return Language::TYPESCRIPT;
        if (ext == ".rs") return Language::RUST;
        if (ext == ".java") return Language::JAVA;
        if (ext == ".cs" || ext == ".csx") return Language::CSHARP;
        if (ext == ".rb" || ext == ".rake" || ext == ".gemspec") return Language::RUBY;
        if (ext == ".php" || ext == ".phtml") return Language::PHP;
        if (ext == ".swift") return Language::SWIFT;
        if (ext == ".kt" || ext == ".kts") return Language::KOTLIN;
        if (ext == ".scala" || ext == ".sc") return Language::SCALA;
        if (ext == ".lua") return Language::LUA;
        if (ext == ".sh" || ext == ".bash" || ext == ".zsh" || ext == ".fish") return Language::SHELL;
        if (ext == ".sql") return Language::SQL;
        if (ext == ".html" || ext == ".htm" || ext == ".vue" || ext == ".svelte") return Language::HTML;
        if (ext == ".xml" || ext == ".xhtml" || ext == ".svg" || ext == ".xsd" || ext == ".xsl") return Language::XML;
        if (ext == ".css" || ext == ".scss" || ext == ".sass" || ext == ".less") return Language::CSS;
        if (ext == ".json" || ext == ".jsonc") return Language::JSON;
        if (ext == ".yaml" || ext == ".yml") return Language::YAML;
        if (ext == ".toml") return Language::TOML;
        if (ext == ".md" || ext == ".markdown") return Language::MARKDOWN;
        if (ext == ".cmake") return Language::CMAKE;
        if (ext == ".mk") return Language::MAKEFILE;
        if (ext == ".vim" || ext == ".vimrc") return Language::VIMSCRIPT;
        if (ext == ".zig") return Language::ZIG;
        if (ext == ".ex" || ext == ".exs") return Language::ELIXIR;
        if (ext == ".pl" || ext == ".pm" || ext == ".pod") return Language::PERL;
        if (ext == ".r" || ext == ".rmd") return Language::R;
        if (ext == ".hs" || ext == ".lhs") return Language::HASKELL;
        if (ext == ".f90" || ext == ".f95" || ext == ".f03" || ext == ".f" || ext == ".for") return Language::FORTRAN;
        if (ext == ".nim" || ext == ".nims" || ext == ".nimble") return Language::NIM;
        if (ext == ".proto") return Language::PROTO;
        if (ext == ".ps1" || ext == ".psm1" || ext == ".psd1") return Language::POWERSHELL;
        if (ext == ".dart") return Language::DART;
        if (ext == ".jl") return Language::JULIA;
        if (ext == ".erl" || ext == ".hrl") return Language::ERLANG;
        if (ext == ".clj" || ext == ".cljs" || ext == ".cljc" || ext == ".edn") return Language::CLOJURE;
        if (ext == ".fs" || ext == ".fsx" || ext == ".fsi") return Language::FSHARP;
    }

    return Language::NONE;
}

std::string SyntaxHighlighter::GetLanguageName(Language lang) {
    switch (lang) {
        case Language::C: return "C";
        case Language::CPP: return "C++";
        case Language::GO: return "Go";
        case Language::PYTHON: return "Python";
        case Language::JAVASCRIPT: return "JavaScript";
        case Language::TYPESCRIPT: return "TypeScript";
        case Language::RUST: return "Rust";
        case Language::JAVA: return "Java";
        case Language::CSHARP: return "C#";
        case Language::RUBY: return "Ruby";
        case Language::PHP: return "PHP";
        case Language::SWIFT: return "Swift";
        case Language::KOTLIN: return "Kotlin";
        case Language::SCALA: return "Scala";
        case Language::LUA: return "Lua";
        case Language::SHELL: case Language::BASH: return "Shell";
        case Language::SQL: return "SQL";
        case Language::HTML: return "HTML";
        case Language::XML: return "XML";
        case Language::CSS: return "CSS";
        case Language::JSON: return "JSON";
        case Language::YAML: return "YAML";
        case Language::TOML: return "TOML";
        case Language::MARKDOWN: return "Markdown";
        case Language::CMAKE: return "CMake";
        case Language::MAKEFILE: return "Makefile";
        case Language::DOCKERFILE: return "Dockerfile";
        case Language::VIMSCRIPT: return "Vim";
        case Language::ZIG: return "Zig";
        case Language::ELIXIR: return "Elixir";
        case Language::PERL: return "Perl";
        case Language::R: return "R";
        case Language::HASKELL: return "Haskell";
        case Language::FORTRAN: return "Fortran";
        case Language::NIM: return "Nim";
        case Language::PROTO: return "Proto";
        case Language::MESON: return "Meson";
        case Language::POWERSHELL: return "PowerShell";
        case Language::DART: return "Dart";
        case Language::JULIA: return "Julia";
        case Language::ERLANG: return "Erlang";
        case Language::CLOJURE: return "Clojure";
        case Language::FSHARP: return "F#";
        default: return "";
    }
}

// ---- 缓存 ----

size_t SyntaxHighlighter::ComputeCacheKey(Language lang, const std::string& line) const {
    size_t h = std::hash<int>{}(static_cast<int>(lang));
    h ^= std::hash<std::string>{}(line) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

// ---- Token 颜色 (主题感知) ----

ftxui::Color SyntaxHighlighter::GetTokenColor(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:     return TC("syn_keyword");
        case TokenType::STRING:      return TC("syn_string");
        case TokenType::COMMENT:     return TC("syn_comment");
        case TokenType::NUMBER:      return TC("syn_number");
        case TokenType::FUNCTION:    return TC("syn_function");
        case TokenType::TYPE:        return TC("syn_type");
        case TokenType::OPERATOR:    return TC("syn_operator");
        case TokenType::PREPROCESSOR:return TC("syn_preprocessor");
        case TokenType::IDENTIFIER:  return TC("syn_identifier");
        case TokenType::PUNCTUATION: return TC("syn_punctuation");
        case TokenType::PROPERTY:    return TC("syn_property");
        case TokenType::TAG:         return TC("syn_tag");
        case TokenType::ATTRIBUTE:   return TC("syn_attribute");
        case TokenType::REGEX:       return TC("syn_regex");
        case TokenType::DECORATOR:   return TC("syn_decorator");
        case TokenType::NORMAL:
        default:                     return TC("syn_identifier");
    }
}

// ---- 解析行 ----

std::vector<SyntaxToken> SyntaxHighlighter::ParseLine(const std::string& line) {
    if (current_language_ == Language::NONE || line.empty()) {
        return {SyntaxToken(line, TokenType::NORMAL, 0, static_cast<int>(line.length()))};
    }

    // 检查缓存
    auto cache_key = ComputeCacheKey(current_language_, line);
    auto cache_it = token_cache_.find(cache_key);
    if (cache_it != token_cache_.end()) {
        return cache_it->second;
    }

    std::vector<SyntaxToken> tokens;
    tokens.reserve(line.length() / 4);

    size_t i = 0;

    // 如果在多行注释中，整行都是注释
    if (in_multiline_comment_) {
        // 查找注释结束
        auto end_pos = line.find("*/");
        if (end_pos != std::string::npos) {
            in_multiline_comment_ = false;
            tokens.emplace_back(line.substr(0, end_pos + 2), TokenType::COMMENT, 0, static_cast<int>(end_pos + 2));
            i = end_pos + 2;
        } else {
            tokens.emplace_back(line, TokenType::COMMENT, 0, static_cast<int>(line.length()));
            token_cache_[cache_key] = tokens;
            return tokens;
        }
    }

    while (i < line.length()) {
        char c = line[i];

        // 多行注释开始
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '*') {
            auto end_pos = line.find("*/", i + 2);
            if (end_pos != std::string::npos) {
                tokens.emplace_back(line.substr(i, end_pos - i + 2), TokenType::COMMENT,
                    static_cast<int>(i), static_cast<int>(end_pos + 2));
                i = end_pos + 2;
            } else {
                in_multiline_comment_ = true;
                tokens.emplace_back(line.substr(i), TokenType::COMMENT,
                    static_cast<int>(i), static_cast<int>(line.length()));
                break;
            }
            continue;
        }

        // 预处理指令 (C/C++: #开头)
        if (IsPreprocessor(line, i, current_language_)) {
            tokens.emplace_back(line.substr(i), TokenType::PREPROCESSOR,
                static_cast<int>(i), static_cast<int>(line.length()));
            break;
        }

        // 装饰器 (Python: @开头)
        if (IsDecorator(line, i, current_language_)) {
            size_t end = i + 1;
            while (end < line.length() && (std::isalnum(line[end]) || line[end] == '_' || line[end] == '.')) {
                end++;
            }
            tokens.emplace_back(line.substr(i, end - i), TokenType::DECORATOR,
                static_cast<int>(i), static_cast<int>(end));
            i = end;
            continue;
        }

        // 注释
        if (IsComment(line, i, current_language_)) {
            tokens.emplace_back(line.substr(i), TokenType::COMMENT,
                static_cast<int>(i), static_cast<int>(line.length()));
            break;
        }

        // 字符串
        if (IsString(line, i)) {
            size_t string_end = FindStringEnd(line, i);
            tokens.emplace_back(line.substr(i, string_end - i), TokenType::STRING,
                static_cast<int>(i), static_cast<int>(string_end));
            i = string_end;
            continue;
        }

        // 数字 (含十六进制、二进制、浮点数后缀)
        if (std::isdigit(c) || (c == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            // 十六进制
            if (c == '0' && i + 1 < line.length() && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
                i += 2;
                while (i < line.length() && (std::isxdigit(line[i]) || line[i] == '_')) i++;
            }
            // 二进制
            else if (c == '0' && i + 1 < line.length() && (line[i + 1] == 'b' || line[i + 1] == 'B')) {
                i += 2;
                while (i < line.length() && (line[i] == '0' || line[i] == '1' || line[i] == '_')) i++;
            }
            // 八进制 / 十进制
            else {
                while (i < line.length() && (std::isdigit(line[i]) || line[i] == '_')) i++;
                if (i < line.length() && line[i] == '.') {
                    i++;
                    while (i < line.length() && (std::isdigit(line[i]) || line[i] == '_')) i++;
                }
                if (i < line.length() && (line[i] == 'e' || line[i] == 'E')) {
                    i++;
                    if (i < line.length() && (line[i] == '+' || line[i] == '-')) i++;
                    while (i < line.length() && (std::isdigit(line[i]) || line[i] == '_')) i++;
                }
            }
            // 数字后缀 (f, F, l, L, u, U, i, j 等)
            while (i < line.length() && (std::isalpha(line[i]) || line[i] == '_')) i++;
            tokens.emplace_back(line.substr(start, i - start), TokenType::NUMBER,
                static_cast<int>(start), static_cast<int>(i));
            continue;
        }

        // 标识符/关键字/类型
        if (std::isalpha(c) || c == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            std::string word = line.substr(start, i - start);

            TokenType type = TokenType::IDENTIFIER;
            if (IsKeyword(word, current_language_)) {
                type = TokenType::KEYWORD;
            } else if (IsType(word, current_language_)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            }

            tokens.emplace_back(word, type, static_cast<int>(start), static_cast<int>(i));
            continue;
        }

        // 运算符
        if (IsOperator(c)) {
            size_t start = i;
            // 多字符运算符
            if (i + 2 < line.length()) {
                std::string tri = line.substr(i, 3);
                if (tri == "<<=" || tri == ">>=" || tri == "<=>" || tri == "===" || tri == "!==" ||
                    tri == "&&=" || tri == "||=" || tri == "?\?=") {
                    tokens.emplace_back(tri, TokenType::OPERATOR, static_cast<int>(start), static_cast<int>(start + 3));
                    i += 3;
                    continue;
                }
            }
            if (i + 1 < line.length()) {
                std::string bi = line.substr(i, 2);
                if (bi == "==" || bi == "!=" || bi == "<=" || bi == ">=" || bi == "&&" || bi == "||" ||
                    bi == "<<" || bi == ">>" || bi == "++" || bi == "--" || bi == "->" || bi == "::" ||
                    bi == "+=" || bi == "-=" || bi == "*=" || bi == "/=" || bi == "%=" || bi == "&=" ||
                    bi == "|=" || bi == "^=" || bi == "=>" || bi == "??" || bi == ".." || bi == "&&" ||
                    bi == "||") {
                    tokens.emplace_back(bi, TokenType::OPERATOR, static_cast<int>(start), static_cast<int>(start + 2));
                    i += 2;
                    continue;
                }
            }
            tokens.emplace_back(std::string(1, c), TokenType::OPERATOR,
                static_cast<int>(start), static_cast<int>(start + 1));
            i++;
            continue;
        }

        // 标点符号
        tokens.emplace_back(std::string(1, c), TokenType::PUNCTUATION,
            static_cast<int>(i), static_cast<int>(i + 1));
        i++;
    }

    // 限制缓存大小
    if (token_cache_.size() > 1000) {
        token_cache_.clear();
    }
    token_cache_[cache_key] = tokens;
    return tokens;
}

// ---- 渲染行 ----

ftxui::Element SyntaxHighlighter::RenderLine(const std::string& line, bool is_cursor_line, int cursor_pos) {
    // 判断 tree-sitter 是否可用且支持当前语言
    bool use_tree_sitter = (backend_ == HighlightBackend::TREE_SITTER
        && SyntaxHighlighterTreeSitter::IsAvailable()
        && ts_highlighter_->HasLanguageSupport());

    // 非光标行: tree-sitter 快速路径（直接返回渲染结果）
    if (use_tree_sitter && !is_cursor_line) {
        return ts_highlighter_->HighlightLine(line);
    }

    // 光标行 + tree-sitter: 获取 tree-sitter token 用于光标渲染
    std::vector<SyntaxToken> tokens;
    bool tokens_from_ts = false;
    if (use_tree_sitter && is_cursor_line) {
        ts_highlighter_->ParseToTokens(line, tokens);
        tokens_from_ts = true;
    }

    // 无 tree-sitter token 时回退到原生解析
    if (tokens.empty() && current_language_ != Language::NONE) {
        tokens = ParseLine(line);
        tokens_from_ts = false;
    }

    // 无语言 + 无 tree-sitter: 纯文本渲染（含光标）
    if (current_language_ == Language::NONE && !use_tree_sitter) {
        if (is_cursor_line && cursor_pos >= 0 && cursor_pos <= static_cast<int>(line.length())) {
            std::string left = line.substr(0, cursor_pos);
            // Get the full UTF-8 character at cursor position
            std::string cursor_char = " ";
            if (cursor_pos < static_cast<int>(line.length())) {
                int char_len = FTB::UnicodeUtil::Utf8CharLen(
                    static_cast<unsigned char>(line[cursor_pos]));
                char_len = std::min(char_len, static_cast<int>(line.length()) - cursor_pos);
                cursor_char = line.substr(cursor_pos, char_len);
            }
            // Skip the current character's bytes for the right part
            int right_start = cursor_pos + static_cast<int>(cursor_char.size());
            if (right_start < cursor_pos) right_start = cursor_pos + 1; // safety
            std::string right = (right_start < static_cast<int>(line.length()))
                ? line.substr(right_start) : "";
            return hbox({
                text(left),
                text(cursor_char) | inverted | bold,
                text(right),
            });
        }
        return text(line) | color(TC("main_fg"));
    }

    // 统一 token 渲染（支持 tree-sitter 和原生 token，含块状光标）
    Elements elements;
    elements.reserve(tokens.size() + 1);

    for (const auto& token : tokens) {
        if (is_cursor_line && token.start_pos <= cursor_pos && token.end_pos > cursor_pos) {
            int offset = cursor_pos - token.start_pos;
            std::string before = token.text.substr(0, offset);
            // Get the full UTF-8 character at the cursor position
            int char_len = 1;
            if (offset < static_cast<int>(token.text.size())) {
                char_len = FTB::UnicodeUtil::Utf8CharLen(
                    static_cast<unsigned char>(token.text[offset]));
                char_len = std::min(char_len, static_cast<int>(token.text.size()) - offset);
            }
            std::string cursor_char = token.text.substr(offset, char_len);
            std::string after = (offset + char_len < static_cast<int>(token.text.size()))
                ? token.text.substr(offset + char_len) : "";
            if (!before.empty()) {
                elements.emplace_back(text(before) | color(GetTokenColor(token.type)));
            }
            elements.emplace_back(text(cursor_char) | inverted | bold);
            if (!after.empty()) {
                elements.emplace_back(text(after) | color(GetTokenColor(token.type)));
            }
        } else {
            elements.emplace_back(text(token.text) | color(GetTokenColor(token.type)));
        }
    }

    // 行末块状光标
    if (is_cursor_line && cursor_pos >= static_cast<int>(line.length())) {
        elements.emplace_back(text(" ") | inverted | bold);
    }

    return hbox(elements);
}

// ---- 辅助函数 ----

bool SyntaxHighlighter::IsKeyword(const std::string& word, Language lang) {
    auto it = keyword_patterns_.find(lang);
    if (it != keyword_patterns_.end()) {
        return it->second.count(word) > 0;
    }
    return false;
}

bool SyntaxHighlighter::IsType(const std::string& word, Language lang) {
    auto it = type_patterns_.find(lang);
    if (it != type_patterns_.end()) {
        return it->second.count(word) > 0;
    }
    return false;
}

bool SyntaxHighlighter::IsNumber(const std::string& word) {
    if (word.empty()) return false;
    size_t start = 0;
    if (word.size() > 2 && word[0] == '0' && (word[1] == 'x' || word[1] == 'X')) start = 2;
    for (size_t i = start; i < word.length(); ++i) {
        if (!std::isdigit(word[i]) && word[i] != '.' && word[i] != '_') return false;
    }
    return true;
}

bool SyntaxHighlighter::IsString(const std::string& line, size_t pos) {
    return pos < line.length() && (line[pos] == '"' || line[pos] == '\'' || line[pos] == '`');
}

bool SyntaxHighlighter::IsComment(const std::string& line, size_t pos, Language lang) {
    if (pos >= line.length()) return false;
    // # 注释: Python, Ruby, Perl, YAML, TOML, Shell
    if (lang == Language::PYTHON || lang == Language::RUBY || lang == Language::PERL ||
        lang == Language::YAML || lang == Language::TOML || lang == Language::SHELL ||
        lang == Language::BASH || lang == Language::R || lang == Language::NIM ||
        lang == Language::JULIA) {
        return line[pos] == '#';
    }
    // // 注释: C, C++, Go, Java, Rust, JS, TS, C#, Swift, Kotlin, Scala, Zig, Dart, F#
    if (lang == Language::C || lang == Language::CPP || lang == Language::GO ||
        lang == Language::JAVA || lang == Language::RUST || lang == Language::JAVASCRIPT ||
        lang == Language::TYPESCRIPT || lang == Language::CSHARP || lang == Language::SWIFT ||
        lang == Language::KOTLIN || lang == Language::SCALA || lang == Language::ZIG ||
        lang == Language::DART || lang == Language::FSHARP) {
        return pos + 1 < line.length() && line[pos] == '/' && line[pos + 1] == '/';
    }
    // -- 注释: SQL, Haskell, Lua, Elixir
    if (lang == Language::SQL || lang == Language::HASKELL || lang == Language::LUA ||
        lang == Language::ELIXIR) {
        return pos + 1 < line.length() && line[pos] == '-' && line[pos + 1] == '-';
    }
    // ; 注释: Lisp, ASM, INI
    if (lang == Language::VIMSCRIPT || lang == Language::MESON) {
        return line[pos] == '#';
    }
    // Dockerfile: # 注释
    if (lang == Language::DOCKERFILE) {
        return line[pos] == '#';
    }
    // CMake: # 注释
    if (lang == Language::CMAKE) {
        return line[pos] == '#';
    }
    // Erlang: % 注释
    if (lang == Language::ERLANG) {
        return line[pos] == '%';
    }
    // Clojure: ; 注释
    if (lang == Language::CLOJURE) {
        return line[pos] == ';';
    }
    return false;
}

size_t SyntaxHighlighter::FindStringEnd(const std::string& line, size_t start) {
    if (start >= line.length()) return start;
    char quote = line[start];
    size_t end = start + 1;
    while (end < line.length() && line[end] != quote) {
        if (line[end] == '\\' && end + 1 < line.length()) {
            end += 2;
        } else {
            end++;
        }
    }
    return (end < line.length()) ? end + 1 : line.length();
}

size_t SyntaxHighlighter::FindCommentEnd(const std::string& line, size_t /*start*/, Language lang) {
    // 单行注释都到行末
    (void)lang;
    return line.length();
}

bool SyntaxHighlighter::IsPreprocessor(const std::string& line, size_t pos, Language lang) {
    if (lang != Language::C && lang != Language::CPP) return false;
    return pos == 0 && line[pos] == '#';
}

bool SyntaxHighlighter::IsOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' ||
           c == '!' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' ||
           c == '~' || c == ':' || c == '.';
}

bool SyntaxHighlighter::IsDecorator(const std::string& line, size_t pos, Language lang) {
    if (lang != Language::PYTHON && lang != Language::RUBY) return false;
    return pos == 0 && line[pos] == '@';
}

} // namespace Editor
} // namespace FTB
