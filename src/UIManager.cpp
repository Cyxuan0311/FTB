#include "UIManager.hpp"
#include "FileManager.hpp"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <mutex>
#include <stack>
#include <thread>

namespace fs = std::filesystem;
using namespace ftxui;

namespace UIManager {

    bool handleEvents(ftxui::Event event, std::stack<std::string>& pathHistory,
                      std::string& currentPath,
                      std::vector<std::string>& allContents,
                      std::vector<std::string>& filteredContents, int& selected,
                      std::string& searchQuery, ScreenInteractive& screen,
                      std::atomic<bool>& refresh_ui) {
        try {
            // 处理 ESC 键事件
            if (event == Event::Escape) {
                refresh_ui = false;
                screen.Exit();
                return true;
            }
            // 处理 Ctrl+P 事件（预览文件内容）
            if (event == Event::CtrlP) {
                if (selected >= 0 &&
                    selected < static_cast<int>(filteredContents.size())) {
                    fs::path fullPath =
                        fs::path(currentPath) / filteredContents[selected];

                    if (!fs::is_regular_file(fullPath)) {
                        std::cerr << "❗ 选中的项目不是文件。" << std::endl;
                        return true;
                    }

                    size_t startLine = 1, endLine = 20; // 默认 1-20 行
                    std::string inputStart, inputEnd;
                    bool cancelled = false; // 标记是否取消

                    // 修改输入提示，添加 emoji
                    auto startInput = Input(&inputStart, "🔢 起始行");
                    auto endInput = Input(&inputEnd, "🔢 结束行");

                    auto confirmButton = Button("✅ 确定", [&] {
                        try {
                            startLine = std::stoul(inputStart);
                            endLine = std::stoul(inputEnd);
                            screen.Exit();
                        } catch (...) {
                            inputStart = "错误";
                            inputEnd = "错误";
                        }
                    });

                    auto cancelButton = Button("❌ 取消", [&] {
                        cancelled = true;
                        screen.Exit(); // 退出输入界面，不进行任何操作
                    });

                    auto container = Container::Vertical(
                        {startInput, endInput, confirmButton, cancelButton});
                    auto inputUI = Renderer(container, [&] {
                        return vbox({
                                   text("📄 请输入预览的行数范围:") | bold | color(Color::CadetBlue),
                                   hbox({text("🔢 起始行: ") | color(Color::CadetBlue),
                                         startInput->Render()}) |
                                       center,
                                   hbox({text("🔢 结束行: ") | color(Color::CadetBlue),
                                         endInput->Render()}) |
                                       center,
                                   hbox({confirmButton->Render() | color(Color::CadetBlue), text(" "),
                                         cancelButton->Render() | color(Color::CadetBlue)}) |
                                       center,
                               }) | borderDouble | color(Color::GrayLight) | center;
                    });

                    screen.Loop(inputUI); // 进入输入界面

                    if (cancelled)
                        return true; // 取消输入，则直接返回

                    std::string fileContent = FileManager::readFileContent(
                        fullPath.string(), startLine, endLine);

                    int scrollOffset = 0; // 滚动偏移量
                    int maxScroll = std::max(
                        0, static_cast<int>(std::count(
                               fileContent.begin(), fileContent.end(), '\n')) - 10); // 计算最大滚动值

                    auto scrollSlider = Slider("🔄 滚动", &scrollOffset, 0, maxScroll, 1) | color(Color::Orange1);
                    auto exitButton = Button("🚪退出", [&] { screen.Exit(); });

                    auto contentRenderer = Renderer([&] {
                        // 将文件内容按行拆分
                        std::vector<std::string> lines;
                        std::istringstream stream(fileContent);
                        std::string line;
                        while (std::getline(stream, line)) {
                            lines.push_back(line);
                        }

                        int visibleLines = 10; // 只显示 10 行
                        std::vector<Element> textElements;
                        // 裁剪出当前滚动范围内的行
                        for (int i = scrollOffset;
                             i < std::min(scrollOffset + visibleLines,
                                          static_cast<int>(lines.size()));
                             i++) {
                            textElements.push_back(text(lines[i]));
                        }

                        return vbox({
                                   text("📄 文件内容预览: " + fullPath.filename().string()) |
                                       bold | borderDouble | color(Color::Green),
                                   vbox(textElements) | borderDouble |
                                       color(Color::GreenYellow) |
                                       size(WIDTH, EQUAL, 150) |
                                       size(HEIGHT, EQUAL, 15),
                                   scrollSlider->Render(),
                                   exitButton->Render() | borderLight | size(WIDTH, EQUAL, 10) | center,
                               }) | center;
                    });

                    // 处理鼠标滚动
                    auto eventHandler =
                        CatchEvent(contentRenderer, [&](Event event) {
                            if (event == Event::Return) {
                                screen.Exit();
                                return true;
                            }
                            if (event.is_mouse()) {
                                auto mouse = event.mouse();
                                if (mouse.button == Mouse::WheelUp) {
                                    scrollOffset = std::max(0, scrollOffset - 1);
                                    return true;
                                }
                                if (mouse.button == Mouse::WheelDown) {
                                    scrollOffset = std::min(maxScroll, scrollOffset + 1);
                                    return true;
                                }
                            }
                            return false;
                        });

                    screen.Loop(eventHandler); // 进入预览界面
                }
                return true;
            }

            // 处理空格键事件，显示文件夹详细信息
            if (event == Event::Character(' ')) {
                std::string targetPath;
                if (selected >= 0 &&
                    selected < static_cast<int>(filteredContents.size())) {
                    fs::path fullPath =
                        fs::path(currentPath) / filteredContents[selected];
                    if (FileManager::isDirectory(fullPath.string())) {
                        targetPath = fullPath.string();
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }

                int fileCount = 0;
                int folderCount = 0;
                std::vector<std::tuple<std::string, mode_t>> folderPermissions;
                std::vector<std::string> fileNames; // 存储文件名的 vector

                // 获取文件夹内的信息
                FileManager::calculation_current_folder_files_number(
                    targetPath, fileCount, folderCount, folderPermissions,
                    fileNames);

                // 退出按钮
                auto exitButton = Button("🚪 退出", [&] { screen.Exit(); });

                auto infoContainer = Container::Vertical({exitButton});

                auto infoComponent = Renderer(infoContainer, [&] {
                    std::vector<Element> elements = {
                        text("📂 文件夹信息") | color(Color::GrayLight) | bold | borderHeavy | center | color(Color::Green3),
                        hbox({text("📍 目标路径: ") | bold | color(Color::GrayLight),
                              text(targetPath) | underlined | color(Color::Orange1)}),
                        hbox({text("📁 文件夹数: ") | bold | color(Color::GrayLight),
                              text(std::to_string(folderCount)) | color(Color::Orange1)}),
                        hbox({text("📄 文件数: ") | bold | color(Color::GrayLight),
                              text(std::to_string(fileCount)) | color(Color::Orange1)}),
                        separator(),
                        text("📌 文件/文件夹列表:") | bold | color(Color::BlueLight)
                    };

                    // 遍历文件列表，区分文件和文件夹
                    for (const auto& name : fileNames) {
                        fs::path filePath = fs::path(targetPath) / name;
                        bool isDir = FileManager::isDirectory(filePath.string());
                        elements.push_back(hbox(
                            {text(isDir ? "📂 " : "📄 ") | color(Color::Yellow1),
                             text(name) | color(Color::White)}));
                    }

                    elements.push_back(separator());
                    std::string shieldEmoji = u8"\U0001F6E1"; // 🛡（不带 FE0F）
                    elements.push_back(
                        text(shieldEmoji + " 文件夹权限信息:") | bold | color(Color::BlueLight)
                    );

                    // 遍历权限信息，仅对文件夹显示
                    for (const auto& [name, mode] : folderPermissions) {
                        elements.push_back(
                            hbox({text("📂 ") | bold | color(Color::Yellow1),
                                  text(name) | underlined}));
                        elements.push_back(hbox(
                            {text("   🛡 权限: ") | bold | color(Color::GrayLight),
                             text(std::to_string(mode & 0777)) | color(Color::Cyan)}));
                    }

                    elements.push_back(
                        hbox({filler(), exitButton->Render(), filler()}) |
                        center);
                    elements.push_back(text("🚪 按 ENTER 退出") | bold | color(Color::Red3Bis));

                    return vbox(elements) | borderDouble | center |
                           color(Color::RGB(185, 185, 168));
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
                    currentPath = current.has_parent_path()
                                      ? current.parent_path().string()
                                      : pathHistory.top();
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

            // 处理 Ctrl+F 事件（新建文件）
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
                            std::cerr << "❗ Failed to create file: "
                                      << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "❗ Error creating file: " << e.what() << std::endl;
                    }
                }
                return true;
            }

            // 处理 Ctrl+K 事件（新建文件夹）
            if (event == Event::CtrlK) {
                std::string dirName;
                auto dirNameInput = Input(&dirName, "📂 文件夹名");
                auto cancelButton = Button("❌ 取消", [&] { screen.Exit(); });
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
                                std::cerr << "❗ Failed to create directory: "
                                          << fullPath.string() << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "❗ Error creating directory: " << e.what() << std::endl;
                        }
                    }
                    screen.Exit();
                });

                auto container = Container::Vertical({dirNameInput,
                    Container::Horizontal({cancelButton, createButton})});
                auto renderer = Renderer(container, [&] {
                    return vbox(
                               {text("📁 新建文件夹") | bgcolor(Color::Orange4Bis),
                                dirNameInput->Render(),
                                hbox({cancelButton->Render() | color(Color::Orange4Bis),
                                      filler(),
                                      createButton->Render() | color(Color::Orange4Bis)}) | hcenter}) |
                           borderDouble | color(Color::GrayLight) |
                           size(WIDTH, GREATER_THAN, 50) | vcenter | hcenter;
                });
                screen.Loop(renderer);
                return true;
            }
            // 处理 Delete 键事件（删除文件或文件夹）
            if (event == Event::Delete) {
                if (selected >= 0 &&
                    selected < static_cast<int>(filteredContents.size())) {
                    std::string itemName = filteredContents[selected];
                    fs::path fullPath = fs::path(currentPath) / itemName;
                    try {
                        if (FileManager::deleteFileOrDirectory(fullPath.string())) {
                            std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                            allContents = FileManager::getDirectoryContents(currentPath);
                            filteredContents = allContents;
                            FileManager::dir_cache[currentPath].valid = false;
                        } else {
                            std::cerr << "❗ Failed to delete: " << fullPath.string() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "❗ Error deleting: " << e.what() << std::endl;
                    }
                }
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "❗ Error: " << e.what() << std::endl;
        }
        return false;
    }

    std::string showNewFileDialog(ScreenInteractive& screen) {
        std::string fileName;
        std::string fileType;
        auto fileNameInput = Input(&fileName, "📝 文件名");
        auto fileTypeInput = Input(&fileType, "🔤 文件类型");
        auto cancelButton = Button("❌ 取消", [&] { screen.Exit(); });
        auto createButton = Button("✅ 创建", [&] { screen.Exit(); });
        auto container = Container::Vertical(
            {fileNameInput, fileTypeInput,
             Container::Horizontal({cancelButton, createButton})});
        auto renderer = Renderer(container, [&] {
            return vbox({text("🆕 新建文件") | bgcolor(Color::Green3Bis),
                         fileNameInput->Render(), fileTypeInput->Render(),
                         hbox({filler(),
                               cancelButton->Render() | color(Color::Green3Bis),
                               createButton->Render() | color(Color::Green3Bis),
                               filler()}) |
                             size(WIDTH, GREATER_THAN, 30)}) |
                   borderDouble | color(Color::GrayLight) |
                   size(WIDTH, GREATER_THAN, 50) | vcenter | hcenter;
        });
        screen.Loop(renderer);
        if (!fileName.empty() && !fileType.empty()) {
            return fileName + "." + fileType;
        }
        return "";
    }

    std::pair<Component, Component>
    initializeUI(std::string& searchQuery,
                 std::vector<std::string>& filteredContents, int& selected,
                 std::stack<std::string>& pathHistory, std::string& currentPath,
                 std::vector<std::string>& allContents) {

        auto searchInput = Input(&searchQuery, "🔍 搜索...");
        MenuOption menu_option;
        menu_option.on_enter = [&] {
            FileManager::enterDirectory(pathHistory, currentPath,
                                        filteredContents, selected);
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
} // namespace UIManager
