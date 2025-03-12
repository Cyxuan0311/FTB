#include "UIManager.hpp"
#include "FileManager.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <filesystem>
#include <thread>
#include <mutex>
#include <stack>
#include <iostream>

namespace fs = std::filesystem;
using namespace ftxui;

namespace UIManager {

    bool handleEvents(ftxui::Event event, 
                      std::stack<std::string>& pathHistory,
                      std::string& currentPath, 
                      std::vector<std::string>& allContents,
                      std::vector<std::string>& filteredContents,
                      int& selected,
                      std::string& searchQuery,
                      ScreenInteractive& screen,
                      std::atomic<bool>& refresh_ui) {
        try {
            if (event == Event::Escape) {
                refresh_ui = false;
                screen.Exit();
                return true;
            }

            if (event == Event::Character(' ')) {
                // 如果当前选中的项目存在，则判断其是否为目录，否则不处理空格事件
                std::string targetPath;
                if (selected >= 0 && selected < static_cast<int>(filteredContents.size())) {
                    fs::path fullPath = fs::path(currentPath) / filteredContents[selected];
                    if (FileManager::isDirectory(fullPath.string())) {
                        targetPath = fullPath.string();
                    } else {
                        // 如果选中项不是目录，则不做任何处理
                        return false;
                    }
                } else {
                    // 如果没有有效选中项，则不处理
                    return false;
                }
                int fileCount = 0;
                int folderCount = 0;
                // 调用 FileManager 中实现的函数，计算目标目录中的文件数和文件夹数
                FileManager::calculation_current_folder_files_number(targetPath, fileCount, folderCount);
                
                // 定义一个退出按钮，点击后退出信息窗口
                auto exitButton = Button("退出", [&] {
                    screen.Exit();
                });
                // 将退出按钮放入一个容器中，以便使其具备交互能力
                auto infoContainer = Container::Vertical({ exitButton });
                // 创建 Renderer 渲染整个统计信息窗口
                auto infoComponent = Renderer(infoContainer, [&] {
                    return vbox({
                        text("文件夹统计") | bold | center | color(Color::Green3),
                        hbox({
                            text("目标路径: ") | bold | color(Color::GrayLight),
                            text(targetPath) | underlined | color(Color::Orange1)
                        }),
                        hbox({
                            text("文件夹数: ") | bold | color(Color::GrayLight),
                            text(std::to_string(folderCount)) | color(Color::Orange1)
                        }),
                        hbox({
                            text("文件数: ") | bold | color(Color::GrayLight),
                            text(std::to_string(fileCount)) | color(Color::Orange1)
                        }),
                        hbox({
                            filler(), exitButton->Render(), filler()
                        }) | center,
                        text("按下ENTER退出") | bold | color(Color::Red3Bis)
                    }) | borderDouble | center | color(Color::RGB(185,185,168));
                });
                screen.Loop(infoComponent);
                return true;
            }            
            
            // 修改后的返回上一级目录逻辑：
            if (event == Event::Backspace || event == Event::ArrowLeft) {
                // 1. 当搜索框中有关键字时，不触发返回上一级目录，交由搜索输入处理删除字符
                if (!searchQuery.empty()) {
                    return false;
                }
                // 2. 如果当前选中的不是第一个文件/文件夹，则只将选中项重置为第一个
                if (selected != 0) {
                    selected = 0;
                    return true;
                }
                // 3. 当选中第一个文件/文件夹时，尝试返回上一级目录
                fs::path current(currentPath);
                if (current.has_parent_path() || !pathHistory.empty()) {
                    pathHistory.push(currentPath);
                    currentPath = current.has_parent_path() ? current.parent_path().string() : pathHistory.top();
                    currentPath = fs::canonical(currentPath).string();
                    FileManager::dir_cache[currentPath].valid = false;
                    std::thread([&] {
                        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                        allContents = FileManager::getDirectoryContents(currentPath);
                        filteredContents = allContents;
                    }).detach();
                    searchQuery.clear();
                    selected = 0;
                    return true;
                }
            }

            if (event == Event::CtrlF) {
                std::string newFileName = showNewFileDialog(screen);
                if (!newFileName.empty()) {
                    fs::path fullPath = fs::path(currentPath) / newFileName;
                    try {
                        if (FileManager::createFile(fullPath.string())) {
                            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                            allContents = FileManager::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            FileManager::dir_cache[currentPath].valid = false;
                        } else {
                            std::cerr << "Failed to create file: " << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error creating file: " << e.what() << std::endl;
                    }
                }
                return true;
            }

            if (event == Event::CtrlK) {
                std::string dirName;
                auto dirNameInput = Input(&dirName, "文件夹名");
                auto cancelButton = Button("❌ 取消", [&] {
                    screen.Exit();
                });
                auto createButton = Button("✅ 创建", [&] {
                    if (!dirName.empty()) {
                        fs::path fullPath = fs::path(currentPath) / dirName;
                        try {
                            if (FileManager::createDirectory(fullPath.string())) {
                                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                                allContents = FileManager::getDirectoryContents(currentPath);
                                filteredContents = allContents;
                                FileManager::dir_cache[currentPath].valid = false;
                            } else {
                                std::cerr << "Failed to create directory: " << fullPath.string() << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error creating directory: " << e.what() << std::endl;
                        }
                    }
                    screen.Exit();
                });

                auto container = Container::Vertical({
                    dirNameInput,
                    Container::Horizontal({
                        cancelButton,
                        createButton
                    })
                });
                auto renderer = Renderer(container, [&] {
                    return vbox({
                        text("新建文件夹"),
                        dirNameInput->Render(),
                        hbox({
                            cancelButton->Render(),
                            filler(),
                            createButton->Render()
                        }) | hcenter
                    }) | borderRounded | color(Color::Orange4Bis)
                      | size(WIDTH, GREATER_THAN, 50)
                      | vcenter | hcenter;
                });
                screen.Loop(renderer);
                return true;
            }
            if (event == Event::Delete) {
                if (selected >= 0 && selected < static_cast<int>(filteredContents.size())) {
                    std::string itemName = filteredContents[selected];
                    fs::path fullPath = fs::path(currentPath) / itemName;
                    try {
                        if (FileManager::deleteFileOrDirectory(fullPath.string())) {
                            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                            allContents = FileManager::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            FileManager::dir_cache[currentPath].valid = false;
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

    std::string showNewFileDialog(ScreenInteractive& screen) {
        std::string fileName;
        std::string fileType;
        auto fileNameInput = Input(&fileName, "文件名");
        auto fileTypeInput = Input(&fileType, "文件类型");
        auto cancelButton = Button("❌ 取消", [&] {
            screen.Exit();
        });
        auto createButton = Button("✅ 创建", [&] {
            screen.Exit();
        });
        auto container = Container::Vertical({
            fileNameInput,
            fileTypeInput,
            Container::Horizontal({
                cancelButton,
                createButton
            })
        });
        auto renderer = Renderer(container, [&] {
            return vbox({
                text("新建文件"),
                fileNameInput->Render(),
                fileTypeInput->Render(),
                hbox({
                    filler(),
                    cancelButton->Render(),
                    createButton->Render(),
                    filler()
                }) | size(WIDTH, GREATER_THAN, 30)
            }) | borderRounded | color(Color::Green3Bis)
              | size(WIDTH, GREATER_THAN, 50)
              | vcenter | hcenter;
        });
        screen.Loop(renderer);
        if (!fileName.empty() && !fileType.empty()) {
            return fileName + "." + fileType;
        }
        return "";
    }

    std::pair<Component, Component> initializeUI(
        std::string& searchQuery,
        std::vector<std::string>& filteredContents,
        int& selected,
        std::stack<std::string>& pathHistory,
        std::string& currentPath,
        std::vector<std::string>& allContents) {
        
        auto searchInput = Input(&searchQuery, "搜索...");
        MenuOption menu_option;
        menu_option.on_enter = [&] {
            FileManager::enterDirectory(pathHistory, currentPath, filteredContents, selected);
            allContents = FileManager::getDirectoryContents(currentPath);
            filteredContents = allContents;
            searchQuery.clear();
        };
        auto selector = Menu(&filteredContents, &selected, menu_option);
        auto mouse_component = CatchEvent(selector, [&](Event event) {
            if (event.is_mouse() && event.mouse().motion == Mouse::Moved) {
                return true;
            }
            if (event.is_mouse() && event.mouse().button == Mouse::Left &&
                event.mouse().motion == Mouse::Pressed) {
                menu_option.on_enter();
                return true;
            }
            return false;
        });
        auto container = Container::Vertical({searchInput, mouse_component});
        return std::make_pair(container, searchInput);
    }
}
