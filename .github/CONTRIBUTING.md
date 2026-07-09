# Contributing to FTB

Thanks for your interest in contributing! FTB is a terminal file browser built with C++17 and FTXUI.

## Where to Start

| Goal | Where to Go |
|------|-------------|
| Report a bug | [Bug Report Template](ISSUE_TEMPLATE/bug_report.yml) |
| Request a feature | [Feature Request Template](ISSUE_TEMPLATE/feature_request.yml) |
| Ask a question | [Other Template](ISSUE_TEMPLATE/other.yml) or [Discussions](https://github.com/anomalyco/FTB/discussions) |

## Development Setup

### Prerequisites

```bash
# Debian / Ubuntu
sudo apt install cmake g++ libftxui-dev nlohmann-json3-dev libgtest-dev
```

### Build

```bash
git clone https://github.com/anomalyco/FTB.git
cd FTB
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Run Tests

```bash
cd build/tests && ctest --output-on-failure
```

### Optional Dependencies

Enable with CMake flags:

```bash
cmake .. \
  -DFTB_ENABLE_SSH=ON \
  -DFTB_ENABLE_TREE_SITTER=ON \
  -DFTB_ENABLE_LIBCHAFA=ON \
  -DFTB_ENABLE_LIBSIXEL=ON \
  -DFTB_ENABLE_AI=ON
```

## Code Style

- **Language**: C++17
- **Headers**: `#pragma once`
- **Naming**:
  - Classes / structs: `PascalCase` (e.g. `ImageOutputManager`)
  - Functions / variables: `snake_case` (e.g. `GetCached`, `s_overlay_active`)
  - Member variables: `m_` prefix (e.g. `m_mutex`)
  - Static variables: `s_` prefix (e.g. `s_protocol`)
  - Constants / macros: `UPPER_SNAKE_CASE`
- **Formatting**: 4-space indentation, no tabs
- **Includes**: Group in order — standard library, third-party, project headers
- **Comments**: Not required — prefer self-documenting code

## Module Structure

```
src/           include/
├── ai/        ├── ai/
├── browser/   ├── browser/
├── config/    ├── config/
├── core/      ├── core/
├── dialog/    ├── dialog/
├── editor/    ├── editor/
├── ops/       ├── ops/
├── preview/   ├── preview/
├── protocols/ ├── protocols/
├── remote/    ├── remote/
├── renderer/  ├── renderer/
└── utils/     └── utils/
```

Each module's source lives in `src/<module>/` and its public header in `include/<module>/`.

## Commit Messages

Use conventional commits:

```
<type>(<scope>): <description>

[optional body]
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `perf`, `style`
Scope: module name (e.g. `ai`, `preview`, `docker`)

Examples:
```
feat(preview): add PDF text extraction via hygg
fix(editor): resolve crash on empty file open
docs(readme): add Docker section
```

## Pull Request Process

1. Fork the repo and create a feature branch from `master`
2. Make your changes following the code style above
3. Ensure tests pass: `cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . && cd tests && ctest`
4. Keep commits focused and use the conventional commit format
5. Open a PR against `master` with a clear description of the changes
6. Link related issues if applicable

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
