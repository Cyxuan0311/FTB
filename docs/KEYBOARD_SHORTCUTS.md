# FTB Keyboard Shortcuts Guide

This comprehensive guide covers all keyboard shortcuts available in FTB (Terminal File Browser).

## 🗂️ Navigation Shortcuts

### Basic Navigation
- **↑/↓**: Navigate through file list
- **Enter**: Open selected directory or file
- **Backspace/←**: Return to parent directory
- **ESC**: Exit program
- **Tab**: Focus search box

### Advanced Navigation
- **Page Up/Page Down**: Scroll through large directories
- **Home**: Jump to first item
- **End**: Jump to last item

## 🛠️ File Operations

### File and Directory Management
- **Ctrl+f**: Create new file
- **Ctrl+k**: Create new directory
- **Delete**: Remove selected item
- **Alt+n**: Rename selected item
- **Space**: View item attributes and details

### Clipboard Operations
- **Ctrl+t**: Copy selected item
- **Ctrl+x**: Cut selected item
- **Ctrl+n**: Paste items from clipboard
- **Alt+c**: Add selected item to clipboard
- **Alt+g**: Clear clipboard

## 📝 Content Viewing and Editing

### File Content Operations
- **Ctrl+p**: Preview file content with range input
- **Alt+v**: Image/text preview
- **Alt+p**: Video playback (for supported formats)
- **Ctrl+e**: Enter Vim-like editor mode

### Advanced Content Features
- **Ctrl+Shift+p**: Full file preview
- **Ctrl+Shift+e**: Advanced editor features

## 🔗 Remote Connections

### SSH Remote Connection
- **Ctrl+s**: Open SSH connection dialog
  - Enter hostname/IP address
  - Specify port (default: 22)
  - Choose authentication method (password/key)
  - Set remote directory path
  - Connect and browse remote files

### SSH Connection Features
- **Ctrl+Shift+s**: Quick SSH connection with saved settings
- **Ctrl+Alt+s**: SSH connection manager

## 🗄️ Database Management

### MySQL Database Operations
- **Alt+d**: Open MySQL database management dialog
  - Configure local or remote MySQL connection
  - Manage databases and tables
  - Execute SQL queries with visual results
  - Perform CRUD operations through buttons

### Database Shortcuts
- **Ctrl+Alt+d**: Quick database connection
- **Ctrl+Shift+d**: Database connection manager

## 🎨 Theme and Configuration

### Theme Management
- **Ctrl+t**: Switch between available themes
- **Ctrl+r**: Reload configuration file
- **Ctrl+Shift+t**: Open theme selector

### Configuration
- **Ctrl+Alt+c**: Open configuration editor
- **Ctrl+Shift+r**: Reset to default configuration

## 🎯 Advanced Features

### Vim-like Editor Mode
When in Vim mode (Ctrl+e), additional shortcuts are available:

#### Navigation
- **h, j, k, l**: Move cursor (left, down, up, right)
- **w, b**: Move by word (forward, backward)
- **0, $**: Move to beginning/end of line
- **gg, G**: Move to beginning/end of file

#### Editing
- **i**: Insert mode
- **a**: Append mode
- **o, O**: New line (below, above)
- **x**: Delete character
- **dd**: Delete line
- **yy**: Copy line
- **p, P**: Paste (below, above)

#### Saving and Exiting
- **:w**: Save file
- **:q**: Quit editor
- **:wq**: Save and quit
- **:q!**: Quit without saving
- **Ctrl+c**: Exit editor mode

### Search and Filter
- **Ctrl+f**: Focus search box
- **Ctrl+Shift+f**: Advanced search options
- **Ctrl+Alt+f**: Filter by file type

## 🖱️ Mouse Support

### Mouse Operations
- **Left Click**: Select item
- **Double Click**: Open item
- **Right Click**: Context menu (if available)
- **Scroll**: Navigate through list

## 🔧 System Shortcuts

### Application Control
- **Ctrl+q**: Quit application
- **Ctrl+Shift+q**: Force quit
- **F1**: Help/About
- **F2**: Settings
- **F5**: Refresh current directory

### Debug and Development
- **Ctrl+Shift+d**: Debug mode toggle
- **Ctrl+Alt+i**: Show system information
- **Ctrl+Shift+i**: Performance metrics

## 📊 Status and Information

### Information Display
- **Ctrl+i**: Show item information
- **Ctrl+Shift+i**: Detailed file statistics
- **Ctrl+Alt+i**: System resource usage

## 🎨 Customization Shortcuts

### Layout and Display
- **Ctrl+Plus**: Increase font size
- **Ctrl+Minus**: Decrease font size
- **Ctrl+0**: Reset font size
- **Ctrl+Shift+l**: Toggle layout mode

### Color and Theme
- **Ctrl+Shift+c**: Color picker
- **Ctrl+Alt+t**: Theme preview
- **Ctrl+Shift+t**: Custom theme editor

## 🚀 Performance Shortcuts

### Caching and Optimization
- **Ctrl+Shift+c**: Clear cache
- **Ctrl+Alt+r**: Refresh cache
- **Ctrl+Shift+r**: Force refresh

### Background Operations
- **Ctrl+Alt+b**: Show background tasks
- **Ctrl+Shift+b**: Background task manager

## 📝 Tips and Best Practices

### Efficient Navigation
1. Use **Tab** to quickly focus the search box
2. Use **Ctrl+f** to create files quickly
3. Use **Ctrl+k** for directory creation
4. Use **Space** to quickly view file details

### File Management
1. Use **Ctrl+t** and **Ctrl+x** for copy/cut operations
2. Use **Ctrl+n** to paste multiple items
3. Use **Alt+c** to add items to clipboard without cutting
4. Use **Alt+g** to clear clipboard when done

### Remote Operations
1. Use **Ctrl+s** for quick SSH connections
2. Use **Alt+d** for database management
3. Save connection settings for faster access

### Theme Customization
1. Use **Ctrl+t** to cycle through themes
2. Use **Ctrl+r** to reload configuration
3. Use **Ctrl+Shift+t** for advanced theme options

## 🔍 Troubleshooting

### Common Issues
- **Shortcuts not working**: Check if another application is using the same keys
- **Vim mode issues**: Press **ESC** to exit insert mode, then use **:q** to quit
- **Connection problems**: Use **Ctrl+Alt+s** or **Ctrl+Alt+d** for connection management

### Reset Options
- **Ctrl+Shift+r**: Reset to default configuration
- **Ctrl+Alt+r**: Reset cache and temporary files
- **F5**: Refresh and reload current state

## 📚 Additional Resources

- [Configuration Guide](CONFIGURATION.md) - Detailed configuration options
- [Project Structure](PROJECT_STRUCTURE.md) - Understanding the codebase
- [Main README](../README.md) - Project overview and features

---

*Note: Some shortcuts may vary depending on your terminal emulator and system configuration. This guide covers the standard FTB shortcuts.*
