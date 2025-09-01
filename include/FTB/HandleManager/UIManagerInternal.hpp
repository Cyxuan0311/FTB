#ifndef UI_MANAGER_INTERNAL_HPP
#define UI_MANAGER_INTERNAL_HPP

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "UI/RenameDialog.hpp"
#include "UI/NewFileDialog.hpp"
#include "UI/NewFolderDialog.hpp"
#include "UI/FilePreviewDialog.hpp"
#include "UI/FolderDetailsDialog.hpp"
#include "UI/SSHDialog.hpp"

#include "FTB/ClipboardManager.hpp"
#include "FTB/DirectoryHistory.hpp"
#include "FTB/FileManager.hpp"
#include "FTB/Vim_Like.hpp"
#include "Video_and_Photo/ImageViewer.hpp"
#include "Video_and_Photo/VideoPlayer.hpp"
#include "FTB/BinaryFileHandler.hpp"

namespace fs = std::filesystem;

namespace UIManagerInternal {

// 辅助函数：验证 selected 合法性并构造完整路径
std::optional<fs::path> getSelectedFullPath(const std::string& currentPath,
                                              const std::vector<std::string>& filteredContents,
                                              int selected);

// 各事件处理函数声明
bool handleRename(ftxui::Event event, const std::string& currentPath,
                  std::vector<std::string>& allContents,
                  std::vector<std::string>& filteredContents, int selected,
                  ftxui::ScreenInteractive& screen);

bool handleImageTextPreview(ftxui::Event event, const std::string& currentPath,
                              const std::vector<std::string>& filteredContents, int selected,
                              ftxui::ScreenInteractive& screen);

bool handleRangePreview(ftxui::Event event, const std::string& currentPath,
                        const std::vector<std::string>& filteredContents, int selected,
                        ftxui::ScreenInteractive& screen);

bool handleFolderDetails(ftxui::Event event, const std::string& currentPath,
                         const std::vector<std::string>& filteredContents, int selected,
                         ftxui::ScreenInteractive& screen);

bool handleNewFile(ftxui::Event event, const std::string& currentPath,
                   std::vector<std::string>& allContents,
                   std::vector<std::string>& filteredContents, ftxui::ScreenInteractive& screen);

bool handleNewFolder(ftxui::Event event, const std::string& currentPath,
                     std::vector<std::string>& allContents,
                     std::vector<std::string>& filteredContents, ftxui::ScreenInteractive& screen);

bool handleBackNavigation(ftxui::Event event, DirectoryHistory& directoryHistory,
                          std::string& currentPath, std::vector<std::string>& allContents,
                          std::vector<std::string>& filteredContents, int& selected,
                          std::string& searchQuery);

bool handleVimMode(ftxui::Event event, const std::string& currentPath,
                   const std::vector<std::string>& filteredContents, int selected,
                   bool& vim_mode_active, VimLikeEditor*& vimEditor);

bool handleDelete(ftxui::Event event, const std::string& currentPath,
                  const std::vector<std::string>& filteredContents, int selected,
                  std::vector<std::string>& allContents);

bool handleChooseFile(ftxui::Event event, const std::string& currentPath,
                  const std::vector<std::string>& filteredContents, int selected);

bool handleCopy(ftxui::Event event, const std::string& currentPath,
                const std::vector<std::string>& filteredContents, int selected);

bool handleClearClipboard(ftxui::Event event);

bool handleCut(ftxui::Event event, const std::string& currentPath,
               const std::vector<std::string>& filteredContents, int selected);

bool handlePaste(ftxui::Event event, const std::string& currentPath,
                std::vector<std::string>& allContents,
                std::vector<std::string>& filteredContents);

bool handleVideoPlay(ftxui::Event event, 
                const std::string& currentPath,
                const std::vector<std::string>& filteredContents, 
                int selected,
                ftxui::ScreenInteractive& screen);

bool handleSSHConnection(ftxui::Event event,
                        ftxui::ScreenInteractive& screen);

bool handleMySQLConnection(ftxui::Event event,
                          ftxui::ScreenInteractive& screen);

bool handleConfigReload(ftxui::Event event,
                        ftxui::ScreenInteractive& screen);

bool handleThemeSwitch(ftxui::Event event,
                       ftxui::ScreenInteractive& screen);

} // namespace UIManagerInternal

#endif // UI_MANAGER_INTERNAL_HPP
