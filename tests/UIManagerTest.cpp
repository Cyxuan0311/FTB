#include <gtest/gtest.h>
#include "../include/UIManager.hpp"
#include "../include/FileManager.hpp"
#include "../include/DirectoryHistory.hpp"  // 使用新的历史记录模块
#include "../include/FTB/Vim/Vim_Like.hpp"           // 新增：Vim 编辑器头文件
#include <vector>
#include <string>
#include <atomic>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/event.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class UIManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用当前工作目录作为基准，创建一个测试目录 test_directory
        testDir = fs::current_path() / "test_directory";
        fs::create_directories(testDir);

        // 创建 test_directory 内的子目录和文件：
        // 1. 创建文件 file1.txt 和 file2.txt（空文件）
        std::ofstream((testDir / "file1.txt").string()).close();
        std::ofstream((testDir / "file2.txt").string()).close();
        // 2. 创建目录 folder1
        fs::create_directories(testDir / "folder1");

        // 设置测试中所用的目录路径和内容列表
        currentPath = testDir.string();
        selected = 0;
        searchQuery.clear();
        // 假定当前目录下包含两个文件和一个文件夹
        allContents = {"file1.txt", "file2.txt", "folder1"};
        filteredContents = allContents;
        refresh_ui = true;

        // 初始化 Vim 模式状态变量
        vim_mode_active = false;
        vimEditor = nullptr;
    }

    void TearDown() override {
        // 测试完成后，删除整个测试目录及其所有内容
        fs::remove_all(testDir);
    }

    // 测试所用的成员变量
    DirectoryHistory pathHistory;
    std::string currentPath;
    std::vector<std::string> allContents;
    std::vector<std::string> filteredContents;
    int selected;
    std::string searchQuery;
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::FitComponent();
    std::atomic<bool> refresh_ui;
    fs::path testDir;  // 存储测试目录的路径

    // 新增：Vim 模式相关变量
    bool vim_mode_active;
    VimLikeEditor* vimEditor;
};

TEST_F(UIManagerTest, HandleEscapeEvent) {
    bool result = UIManager::handleEvents(
        ftxui::Event::Escape, 
        pathHistory, 
        currentPath, 
        allContents,
        filteredContents, 
        selected, 
        searchQuery, 
        screen, 
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_TRUE(result);
    EXPECT_FALSE(refresh_ui);
}

TEST_F(UIManagerTest, HandleBackspaceEvent_Navigation) {
    // 假设测试从 test_directory 返回上一级，即当前工作目录
    currentPath = (fs::current_path() / "test_directory").string();
    selected = 0;
    bool result = UIManager::handleEvents(
        ftxui::Event::Backspace,
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    // 预期返回 true，且当前路径为 fs::current_path()
    EXPECT_TRUE(result);
    EXPECT_EQ(currentPath, fs::canonical(fs::current_path()).string());
}

TEST_F(UIManagerTest, HandleCharacterEvent_SpaceOnFile) {
    selected = 0; // file1.txt (文件)
    bool result = UIManager::handleEvents(
        ftxui::Event::Character(' '),
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_FALSE(result);
}

TEST_F(UIManagerTest, HandleCharacterEvent_SpaceOnFolder) {
    selected = 2; // folder1 (目录)
    bool result = UIManager::handleEvents(
        ftxui::Event::Character(' '),
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleCtrlF_CreateFile) {
    bool result = UIManager::handleEvents(
        ftxui::Event::CtrlF,
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleCtrlK_CreateDirectory) {
    bool result = UIManager::handleEvents(
        ftxui::Event::CtrlK,
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleDelete_DeleteFile) {
    selected = 0;
    bool result = UIManager::handleEvents(
        ftxui::Event::Delete,
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui,
        vim_mode_active,
        vimEditor
    );
    EXPECT_TRUE(result);
}
