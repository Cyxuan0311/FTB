# FTB Project Structure

This document provides a detailed overview of the FTB project structure, including all directories, files, and their purposes.

## Directory Structure

```
FTB/
├── include/                    # Header files
│   ├── FTB/                   # Core FTB headers
│   │   ├── FileManager.hpp
│   │   ├── ThreadGuard.hpp
│   │   ├── FileSizeCalculator.hpp
│   │   ├── Vim_Like.hpp
│   │   ├── DirectoryHistory.hpp
│   │   ├── detail_element.hpp
│   │   ├── ConfigManager.hpp  # Configuration management
│   │   ├── ThemeManager.hpp   # Theme management
│   │   ├── ObjectPool.hpp     # Object pooling for performance
│   │   ├── AsyncFileManager.hpp # Asynchronous file operations
│   │   ├── NetworkService.hpp # Network service management
│   │   └── HandleManager/
│   │       ├── UIManager.hpp
│   │       └── UIManagerInternal.hpp
│   ├── UI/                    # User interface components
│   │   ├── RenameDialog.hpp
│   │   ├── NewFileDialog.hpp
│   │   ├── NewFolderDialog.hpp
│   │   ├── FilePreviewDialog.hpp
│   │   ├── FolderDetailsDialog.hpp
│   │   ├── SSHDialog.hpp      # SSH connection dialog
│   │   ├── MySQLDialog.hpp    # MySQL database management dialog
│   │   └── NetworkServiceDialog.hpp # Network service management dialog
│   ├── Connection/            # Connection backends
│   │   ├── SSHConnection.hpp  # SSH connection backend
│   │   └── MySQLConnection.hpp # MySQL connection backend
│   └── Video_and_Photo/       # Media handling
│       ├── ImageViewer.hpp
│       └── VideoPlayer.hpp
├── src/                       # Source code
│   ├── FTB/                   # Core FTB implementation
│   │   ├── main.cpp
│   │   ├── FileManager.cpp
│   │   ├── DirectoryHistory.cpp
│   │   ├── detail_element.cpp
│   │   ├── ThreadGuard.cpp
│   │   ├── Vim_Like.cpp
│   │   ├── FileSizeCalculator.cpp
│   │   ├── ConfigManager.cpp  # Configuration management
│   │   ├── ThemeManager.cpp   # Theme management
│   │   ├── ObjectPool.cpp     # Object pooling implementation
│   │   ├── AsyncFileManager.cpp # Asynchronous file operations
│   │   ├── NetworkService.cpp # Network service implementation
│   │   └── HandleManager/
│   │       ├── UIManager.cpp
│   │       └── UIManagerInternal.cpp
│   ├── UI/                    # UI components
│   │   ├── RenameDialog.cpp
│   │   ├── NewFileDialog.cpp
│   │   ├── NewFolderDialog.cpp
│   │   ├── FilePreviewDialog.cpp
│   │   ├── FolderDetailsDialog.cpp
│   │   ├── SSHDialog.cpp      # SSH dialog implementation
│   │   ├── MySQLDialog.cpp    # MySQL dialog implementation
│   │   └── NetworkServiceDialog.cpp # Network service dialog implementation
│   ├── Connection/            # Connection backends
│   │   ├── SSHConnection.cpp  # SSH connection logic
│   │   └── MySQLConnection.cpp # MySQL connection logic
│   └── Video_and_Photo/       # Media handling
│       ├── ImageViewer.cpp
│       ├── VideoPlayer.cpp
│       ├── CommonMedia.cpp
│       └── FFmpegUtils.cpp
├── tests/                     # Test cases
│   ├── UIManagerTest.cpp
│   ├── FileManagerTest.cpp
│   ├── main.cpp
│   ├── Vim_like_Test.cpp
│   ├── CMakeLists.txt
│   └── FileSizeCaculatorTest.cpp
├── config/                    # Configuration files
│   └── .ftb.template         # Configuration template
├── image/                     # Images and media
│   └── demo.png              # Demo screenshot
├── docs/                      # Documentation
│   ├── CONFIGURATION.md      # Configuration guide
│   ├── PROJECT_STRUCTURE.md  # This file
│   └── KEYBOARD_SHORTCUTS.md # Keyboard shortcuts guide
├── bash_tool/                 # Build scripts
│   ├── build.sh
│   ├── package_build.sh
│   └── uninstall.sh
├── .github/workflows/         # CI/CD
│   └── Release.yml
├── CMakeLists.txt            # Build configuration
├── README.md                 # Main documentation
├── QUICK_CONFIG.md           # Quick configuration guide
├── test_config.sh            # Configuration test script
├── .gitignore
├── Release_notes.md
├── cmake_uninstall.cmake.in
├── .clang-format             # Code style
└── License
```

## Core Components

### 🗂️ **Core FTB Module** (`src/FTB/`)
- **FileManager**: File and directory operations with caching
- **DirectoryHistory**: Navigation history management
- **Vim_Like**: Built-in text editor
- **ConfigManager**: Configuration file management
- **ThemeManager**: Theme and color management
- **ObjectPool**: Performance optimization with object pooling
- **AsyncFileManager**: Asynchronous file operations
- **NetworkService**: Network monitoring and management

### 🎨 **User Interface** (`src/UI/`)
- **Dialog Components**: Various input and display dialogs
- **SSH Dialog**: Remote connection interface
- **MySQL Dialog**: Database management interface
- **Network Service Dialog**: Network monitoring and management interface

### 🔗 **Connection Backends** (`src/Connection/`)
- **SSHConnection**: Secure shell remote access
- **MySQLConnection**: Database connectivity

### 🎬 **Media Handling** (`src/Video_and_Photo/`)
- **ImageViewer**: Image preview and display
- **VideoPlayer**: Video playback functionality
- **FFmpegUtils**: Media processing utilities

### 🧪 **Testing** (`tests/`)
- Unit tests for core functionality
- UI component testing
- Performance testing

## Build System

### CMake Configuration
- **Main CMakeLists.txt**: Primary build configuration
- **Test CMakeLists.txt**: Testing framework setup
- **Dependencies**: FTXUI, libssh2, libmysqlclient, FFmpeg

### Build Scripts
- **build.sh**: Main build script
- **package_build.sh**: Package creation
- **uninstall.sh**: Cleanup script

## Documentation Structure

- **README.md**: Main project overview
- **CONFIGURATION.md**: Detailed configuration guide
- **PROJECT_STRUCTURE.md**: This file
- **KEYBOARD_SHORTCUTS.md**: Keyboard shortcuts reference

## Performance Optimizations

The project includes several performance optimization components:

- **ObjectPool**: Reuses frequently created objects
- **AsyncFileManager**: Non-blocking file operations
- **LRU Caching**: Multi-level caching system
- **Smart Pointers**: Automatic memory management
- **Container Pre-allocation**: Reduces memory reallocations

## Development Guidelines

### Code Organization
- Headers in `include/` directory
- Implementation in `src/` directory
- Tests in `tests/` directory
- Documentation in `docs/` directory

### Naming Conventions
- Headers: `*.hpp`
- Source files: `*.cpp`
- Test files: `*Test.cpp`
- Documentation: `*.md`

### Dependencies
- **C++17** or later
- **FTXUI 5.0+**
- **libssh2** for SSH connections
- **libmysqlclient** for database operations
- **FFmpeg** for media handling (optional)
