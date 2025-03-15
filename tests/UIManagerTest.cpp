#include <gtest/gtest.h>
#include "../include/UIManager.hpp"
#include "../include/FileManager.hpp"
#include <stack>
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
    }

    void TearDown() override {
        // 测试完成后，删除整个测试目录及其所有内容
        fs::remove_all(testDir);
    }

    // 测试所需的成员变量
    std::stack<std::string> pathHistory;
    std::string currentPath;
    std::vector<std::string> allContents;
    std::vector<std::string> filteredContents;
    int selected;
    std::string searchQuery;
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::FitComponent();
    std::atomic<bool> refresh_ui;

    fs::path testDir;  // 存储测试目录的路径
};

TEST_F(UIManagerTest, HandleEscapeEvent) {
    bool result = UIManager::handleEvents(ftxui::Event::Escape, pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_TRUE(result);
    EXPECT_FALSE(refresh_ui);
}


/*TEST_F(UIManagerTest, HandleBackspaceEvent_AtRoot) {
    // Get actual root path from test directory
    const fs::path rootPath = fs::canonical(testDir).root_path();
    currentPath = rootPath.string();
    
    // Clear path history completely
    while (!pathHistory.empty()) pathHistory.pop();

    bool result = UIManager::handleEvents(
        ftxui::Event::Backspace,
        pathHistory,
        currentPath,
        allContents,
        filteredContents,
        selected,
        searchQuery,
        screen,
        refresh_ui
    );

    // Use path objects for comparison
    const fs::path actualPath(currentPath);
    EXPECT_FALSE(result);
    EXPECT_EQ(actualPath, rootPath);
    EXPECT_TRUE(pathHistory.empty());
}*/

TEST_F(UIManagerTest, HandleBackspaceEvent_Navigation) {
    // 假设你想测试从 /home/xxx/test_directory 返回上一级
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
        refresh_ui
    );

    // 在真实文件系统中，它的父目录就是 fs::current_path()
    EXPECT_TRUE(result);
    EXPECT_EQ(currentPath, fs::canonical(fs::current_path()).string());
}


TEST_F(UIManagerTest, HandleCharacterEvent_SpaceOnFile) {
    selected = 0; // file1.txt (不是目录)
    bool result = UIManager::handleEvents(ftxui::Event::Character(' '), pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_FALSE(result);
}

TEST_F(UIManagerTest, HandleCharacterEvent_SpaceOnFolder) {
    selected = 2; // folder1 (目录)
    bool result = UIManager::handleEvents(ftxui::Event::Character(' '), pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleCtrlF_CreateFile) {
    bool result = UIManager::handleEvents(ftxui::Event::CtrlF, pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleCtrlK_CreateDirectory) {
    bool result = UIManager::handleEvents(ftxui::Event::CtrlK, pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_TRUE(result);
}

TEST_F(UIManagerTest, HandleDelete_DeleteFile) {
    selected = 0;
    bool result = UIManager::handleEvents(ftxui::Event::Delete, pathHistory, currentPath, allContents,
                                          filteredContents, selected, searchQuery, screen, refresh_ui);
    EXPECT_TRUE(result);
}


