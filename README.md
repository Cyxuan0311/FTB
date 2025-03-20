# FTB (Terminal File Browser)

![C++17](https://img.shields.io/badge/C++-17-blue) ![FTXUI](https://img.shields.io/badge/FTXUI-5.0.0-orange)

![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey) ![Tool](https://img.shields.io/badge/CMake-3.20.0-red)

A terminal-based interactive file browser developed using FTXUI library, providing intuitive directory navigation and file viewing experience.

## Demo

![Demo](https://yt3.ggpht.com/iHL64dUd3WQpbat--V-mzE1PKBu6CLeUyliucuFYF2J8oSZXk3Fn2-aS2v0aQBdrd4CwjP8YWeAh=s1600-rw-nd-v1)

## Features

- 🗂️ Real-time directory content display
- 🎨 Color-coded items (blue for directories/red for files)
- ⏰ Top status bar with current time
- 📁 Path history stack (support backward navigation)
- ↕️ Keyboard navigation (↑↓ keys for selection, Enter to enter directories)
- 🔄 Auto-refresh (100ms UI update interval)
- 🔎 Directory search functionality
- 🧑🎓 Memory usage calculation for selected items
- 🛠️ File/folder creation and deletion
- 👌 Attribute preview and file content inspection

## Dependencies

```bash
# Install FTXUI library
sudo apt-get install libftxui-dev

```

## Build & Run
```bash
chmod +x ./build.sh
source ./build.sh      # Run build script
FTB             # Launch application
```

## Usage
- ↑/↓: Navigate file list
- Enter: Open selected directory
- Backspace: Return to parent directory
- ESC: Exit program
- Search box: Filter files by keyword
- Ctrl+f: Create new file
- Ctrl+k: Create new directory
- Delete: Remove selected item
- Space: View item attributes
- Ctrl+p: Preview file content (excluding binary files like .so, .o, .a)

## Project Structure

    FileBrowser/
    ├── include/            # Header files
    │   ├── FileManager.hpp
    │   ├── ThreadGuard.hpp
    │   └── UIManager.hpp
    ├── src/                # Source code
    │   ├── FileManager.cpp
    │   ├── main.cpp
    │   ├── ThreadGuard.cpp
    │   └── UIManager.cpp
    ├── tests/              # Test cases
    │   └── UIManagerTest.cpp
    ├── CMakeLists.txt      # Build configuration
    ├── README.md           # Documentation
    ├── .gitignore
    ├── build.sh            # Build script
    ├── .clang-format       # Code style
    └── License

## Development Environment
- Compiler: g++ 11.0+
- Build tool: CMake 3.20+
- Dependency: FTXUI 5.0+

## License

MIT License