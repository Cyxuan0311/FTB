# v1.0.0

## Describe:
This is the first version of FTB, which includes features such as browsing current files on the terminal, accessing previous and next level directories, providing file search functionality, as well as the ability to create new files and folders, delete them, and preview file contents and view details of selected folders.
## Attention:
The software just suit the Linux now

# v2.1.0

## Major:
- Complete project restructuring: modular architecture with dedicated modules (browser/, config/, editor/, preview/, dialog/, ops/, remote/, renderer/, utils/, core/, ai/, plugins/)
- Integrated AI assistant panel with Ollama/OpenAI support (`feat(ai)`)
- Extensible plugin system with async preview execution and sample plugins
- SSH remote connection management
- MySQL database management with interactive dialog
- Theme management system with customizable color schemes
- System information monitoring dialog (Device/Status/Disk/Network tabs)
- Network service monitoring and speed testing
- Enhanced Markdown rendering engine (MD_transformer) with inline formatting
- Audio preview support
- Batch rename and task panel dialogs
- LRU cache optimization for file manager
- Nerd Font icon mapping (IconMapper) with extensive folder/file type icons
- Extension-based color mapping for file types
- Fuzzy finder, powerline, and calendar panel
- Async file manager with object pool
- Keyboard shortcuts guide and project structure documentation
- Chinese documentation (README_CN, CONFIGURATION_CN, PLUGINS_CN, PREREQUISITES_CN)
- PerfLogger instrumentation for performance analysis

## Improvements:
- Upgraded FTXUI from v5.0.0 to v6.0.0 (Ctrl+X event support)
- Migrated from raw pointers to smart pointers for memory safety
- Improved file caching with LRU and preloading mechanisms
- Enhanced Markdown display referencing glow design (lists, headers, code blocks, tables)
- Optimized ASCII art rendering for image/video preview
- Streamlined console output by removing verbose logging
- Improved syntax highlighting with broader language support
- Modernized UI panels with consistent file type color rendering
- Enhanced status bar with plugin alignment and config reload feedback

## Fixes:
- Fixed program crashes caused by mouse clicks during execution
- Fixed stdin leak in media preview subprocesses
- Fixed image preview failure handling
- Fixed missing includes for std::function, strlen, std::transform, algorithm
- Fixed mutex mutability for const methods
- Fixed popen attribute warnings with lambda deleter
- Fixed lowercase TS primitive type support in plugin TypeScript compilation
- Fixed leftover continuation lines in multi-line PERF_LOG calls
- Fixed MD_transformer and EventHandler updates

## Build & CI:
- Build FTXUI from source in GitHub Actions (Linux + macOS)
- Added macOS build and test support
- Added libgtest-dev and googletest dependencies
- Updated CMakeLists.txt with MySQL, SSH2, yaml-cpp, TBB library support
- Added httplib, stb_image, QuickJS third-party dependencies
- Introduced build.sh for streamlined builds
- Added uninstall target for clean removal

## Docs:
- Added comprehensive CONFIGURATION.md and CONFIGURATION_CN.md
- Added PLUGINS.md and PLUGINS_CN.md for plugin development
- Added PREREQUISITES.md and PREREQUISITES_CN.md for external dependencies
- Added KEYBOARD_SHORTCUTS.md and KEYBOARD_SHORTCUTS_CN.md
- Modernized README with macOS badge and network service documentation

## Plugins:
- hello-world.ftb - basic plugin example
- file-info.ftb - file metadata display
- git-status.ftb - Git repository status
- sysinfo.ftb - system information collector
- preview-external.ftb - external preview for .md/.pdf/.image files

# v2.0.0

## Fix: 
Fixed program crashes caused by mouse clicks during program execution

## What's new: 
1-Added a sidebar to improve the scalability of the program. 
2-Added an editor similar to Vim to provide online editing. 
3-Add an online calendar for viewing time.

## Suggestion: 
Use v2.0.0 to optimize some features.
