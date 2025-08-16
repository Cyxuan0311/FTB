# FTB (Terminal File Browser)

![C++17](https://img.shields.io/badge/C++-17-blue) ![FTXUI](https://img.shields.io/badge/FTXUI-5.0.0-orange)

![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey) ![Tool](https://img.shields.io/badge/CMake-3.20.0-red) ![SSH](https://img.shields.io/badge/SSH-Supported-green) <a href="#"><img src="https://img.shields.io/github/repo-size/Cyxuan0311/FTB"></img></a>

A powerful terminal-based interactive file browser developed using FTXUI library, providing intuitive directory navigation, file management, and **SSH remote connection** capabilities.

## Demo

![Demo](https://yt3.ggpht.com/F-87fRzFvxPBZSdqt7Wy229FqZfvEiChvp6kpbuCZL7WxfjucfUyyhftxz8V0bTVM_3ZzMVFlNJE=s1600-rw-nd-v1)

## Features

### 🗂️ Core File Management
- Real-time directory content display
- Color-coded items (blue for directories/red for files)
- Top status bar with current time
- Path history stack (support backward navigation)
- Keyboard navigation (↑↓ keys for selection, Enter to enter directories)
- Auto-refresh (100ms UI update interval)
- Directory search functionality
- Memory usage calculation for selected items

### 🛠️ File Operations
- File/folder creation and deletion
- Copy, cut, and paste operations
- File/folder renaming
- Clipboard management
- Attribute preview and file content inspection

### 📝 Advanced Features
- **Vim-Like editing mode** - Built-in text editor
- **Image preview** - Support for JPG, PNG, BMP, GIF
- **Video playback** - MP4, AVI, MKV, MOV, FLV, WMV support
- **Binary file handling** - Smart detection and protection

### 🔗 **SSH Remote Connection** ✨
- **Secure SSH connections** to remote servers
- **Password and key-based authentication**
- **Remote directory browsing**
- **Command execution** on remote hosts
- **Connection status monitoring**
- **Easy-to-use connection dialog**

### 🎨 User Experience
- Modern terminal UI with FTXUI
- Responsive design and smooth animations
- Intuitive keyboard shortcuts
- Error handling and user feedback

## Dependencies

```bash
# Install required libraries
sudo apt-get install libftxui-dev libssh2-1-dev

# For video/image support (optional)
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libx11-dev
```

## Build & Run
```bash
chmod +x ./build.sh
source ./build.sh      # Run build script
FTB             # Launch application
```

## Uninstall(Local)
```bash
./uninstall.sh

```

## Usage

### 🗂️ Navigation
- **↑/↓**: Navigate file list
- **Enter**: Open selected directory
- **Backspace/←**: Return to parent directory
- **ESC**: Exit program
- **Search box**: Filter files by keyword

### 🛠️ File Operations
- **Ctrl+f**: Create new file
- **Ctrl+k**: Create new directory
- **Delete**: Remove selected item
- **Alt+n**: Rename selected item
- **Ctrl+t**: Copy selected item
- **Ctrl+x**: Cut selected item
- **Ctrl+n**: Paste items
- **Alt+c**: Add to clipboard
- **Alt+g**: Clear clipboard

### 📝 Content Viewing
- **Space**: View item attributes
- **Ctrl+p**: Preview file content
- **Alt+v**: Image/text preview
- **Alt+p**: Video playback
- **Ctrl+e**: Vim-like editor

### 🔗 **SSH Remote Connection**
- **Ctrl+s**: Open SSH connection dialog
  - Enter hostname/IP, port, username
  - Choose password or key authentication
  - Specify remote directory
  - Connect and browse remote files

### 🎯 Advanced Features
- **Ctrl+E**: Enter Vim editing mode for text files
- **Mouse support**: Click to select and navigate

## SSH Connection Example

### Quick Start
1. Press `Ctrl+S` to open the SSH connection dialog
2. Fill in the connection details:
   - **Hostname**: `192.168.1.100` or `server.example.com`
   - **Port**: `22` (default SSH port)
   - **Username**: `your_username`
   - **Authentication**: Choose password or private key
   - **Remote Directory**: `/home/username` (default)
3. Click "Confirm" to establish the connection
4. Browse remote files and execute commands

### Supported Authentication Methods
- **Password Authentication**: Enter your SSH password
- **Key Authentication**: Specify path to your private key file

### Security Features
- Encrypted SSH protocol (libssh2)
- Secure password handling
- Key-based authentication support
- Connection status monitoring

## Project Structure

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
│   │   └── HandleManager/
│   │       ├── UIManager.hpp
│   │       └── UIManagerInternal.hpp
│   ├── UI/                    # User interface components
│   │   ├── RenameDialog.hpp
│   │   ├── NewFileDialog.hpp
│   │   ├── NewFolderDialog.hpp
│   │   ├── FilePreviewDialog.hpp
│   │   ├── FolderDetailsDialog.hpp
│   │   └── SSHDialog.hpp      # SSH connection dialog
│   ├── Connection/            # SSH connection backend
│   │   └── SSHConnection.hpp
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
│   │   └── HandleManager/
│   │       ├── UIManager.cpp
│   │       └── UIManagerInternal.cpp
│   ├── UI/                    # UI components
│   │   ├── RenameDialog.cpp
│   │   ├── NewFileDialog.cpp
│   │   ├── NewFolderDialog.cpp
│   │   ├── FilePreviewDialog.cpp
│   │   ├── FolderDetailsDialog.cpp
│   │   └── SSHDialog.cpp      # SSH dialog implementation
│   ├── Connection/            # SSH backend
│   │   └── SSHConnection.cpp  # SSH connection logic
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
├── bash_tool/                 # Build scripts
│   ├── build.sh
│   ├── package_build.sh
│   └── uninstall.sh
├── .github/workflows/         # CI/CD
│   └── Release.yml
├── CMakeLists.txt            # Build configuration
├── README.md                 # Documentation
├── .gitignore
├── Release_notes.md
├── cmake_uninstall.cmake.in
├── .clang-format             # Code style
└── License
```

## Development Environment
- Compiler: g++ 11.0+
- Build tool: CMake 3.20+
- Dependencies: FTXUI 5.0+, libssh2

## License

MIT License