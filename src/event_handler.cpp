#include "../include/event_handler.hpp"
#include <filesystem>
#include <iostream>
#include "../include/file_browser.hpp"  // 确保包含 FileBrowser 相关头文件
#include "../include/file_operations.hpp"
#include "../include/new_file_dialog.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/screen_interactive.hpp>

extern std::mutex cache_mutex;
extern std::unordered_map<std::string, DirectoryCache> dir_cache;

namespace fs = std::filesystem;

//分离了main.cpp的对按键的事务处理。
bool handleEvents(ftxui::Event event, std::stack<std::string>& pathHistory, std::string& currentPath, 
                  std::vector<std::string>& allContents, std::vector<std::string>& filteredContents,
                  int& selected, std::string& searchQuery, ftxui::ScreenInteractive& screen,
                  std::atomic<bool>& refresh_ui) {
    try {
        // 按下 ESC 键时退出：设置标志并调用 screen.Exit()
        if (event == ftxui::Event::Escape) {
            refresh_ui = false;
            screen.Exit();
            return true;
        }
        if (event == ftxui::Event::Backspace || event == ftxui::Event::ArrowLeft) {
            fs::path current(currentPath);
            if (current.has_parent_path() || !pathHistory.empty()) {
                pathHistory.push(currentPath);
                currentPath = current.has_parent_path() ? 
                    current.parent_path().string() : pathHistory.top();

                currentPath = fs::canonical(currentPath).string();
                dir_cache[currentPath].valid = false;

                std::thread([&] {
                    std::lock_guard<std::mutex> lock(cache_mutex);
                    allContents = FileBrowser::getDirectoryContents(currentPath);
                    filteredContents = allContents;
                }).detach();

                searchQuery.clear();
                selected = 0;
                return true;
            }
        }
        if (event == ftxui::Event::Character('&')) {
            auto newFileName = showNewFileDialog(screen);
            if (!newFileName.empty()) {
                fs::path fullPath = fs::path(currentPath) / newFileName;
                try {
                    if (FileOperations::createFile(fullPath.string())) {
                        std::lock_guard<std::mutex> lock(cache_mutex);
                        allContents = FileBrowser::getDirectoryContents(currentPath);
                        filteredContents = allContents;
                        dir_cache[currentPath].valid = false;
                    } else {
                        std::cerr << "Failed to create file: " << fullPath.string() << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error creating file: " << e.what() << std::endl;
                }
            }
            return true;
        }
        if (event == ftxui::Event::Character('^')) {
            std::string dirName;
            auto dirNameInput = ftxui::Input(&dirName, "文件夹名");
            auto cancelButton = ftxui::Button("❌ 取消", [&] {
                screen.Exit();
            });
            auto createButton = ftxui::Button("✅ 创建", [&] {
                if (!dirName.empty()) {
                    fs::path fullPath = fs::path(currentPath) / dirName;
                    try {
                        if (FileOperations::createDirectory(fullPath.string())) {
                            std::lock_guard<std::mutex> lock(cache_mutex);
                            allContents = FileBrowser::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            dir_cache[currentPath].valid = false;
                        } else {
                            std::cerr << "Failed to create directory: " << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error creating directory: " << e.what() << std::endl;
                    }
                }
                screen.Exit();
            });
            auto container = ftxui::Container::Vertical({
                dirNameInput,
                ftxui::Container::Horizontal({
                    cancelButton,
                    createButton
                })
            });
            auto renderer = ftxui::Renderer(container, [&] {
                return ftxui::vbox({
                    ftxui::text("新建文件夹"),
                    dirNameInput->Render(),
                    ftxui::hbox({
                        cancelButton->Render(),
                        ftxui::filler(),
                        createButton->Render()
                    }) | ftxui::hcenter
                }) | ftxui::borderRounded | ftxui::color(ftxui::Color::Orange4Bis) 
                  | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 50) 
                  | ftxui::vcenter | ftxui::hcenter;
            });
            screen.Loop(renderer);
            return true;
        }
        if (event == ftxui::Event::Character('~')) {
            if (selected >= 0 && selected < filteredContents.size()) {
                std::string itemName = filteredContents[selected];
                fs::path fullPath = fs::path(currentPath) / itemName;
                try {
                    if (FileOperations::deleteFileOrDirectory(fullPath.string())) {
                        std::lock_guard<std::mutex> lock(cache_mutex);
                        allContents = FileBrowser::getDirectoryContents(currentPath);
                        filteredContents = allContents;
                        dir_cache[currentPath].valid = false;
                    } else {
                        std::cerr << "Failed to delete: " << fullPath.string() << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error deleting: " << e.what() << std::endl;
                }
            }
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return false;
}