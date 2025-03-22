// test_file_manager.cpp
#include "gtest/gtest.h"
#include "../include/FileManager.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>

namespace fs = std::filesystem;
using namespace FileManager;

// 自定义生成唯一路径的函数
std::string generate_unique_path() {
    // 获取当前时间戳（毫秒）
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // 生成随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    int random_number = dis(gen);

    // 构造唯一目录名称
    std::ostringstream oss;
    oss << "fileman_test_" << ms << "_" << random_number;
    return oss.str();
}

class FileManagerTest : public ::testing::Test {
protected:
    fs::path temp_dir;

    void SetUp() override {
        // 在系统临时目录下创建一个唯一的测试目录
        temp_dir = fs::temp_directory_path() / generate_unique_path();
        fs::create_directory(temp_dir);
    }

    void TearDown() override {
        // 测试结束后删除测试目录及其所有内容
        fs::remove_all(temp_dir);
    }
};

// 测试 isDirectory 函数
TEST_F(FileManagerTest, IsDirectory) {
    // 测试临时目录为目录
    EXPECT_TRUE(isDirectory(temp_dir.string()));

    // 创建一个临时文件
    fs::path temp_file = temp_dir / "temp_file.txt";
    std::ofstream ofs(temp_file.string());
    ofs << "Test content";
    ofs.close();
    EXPECT_FALSE(isDirectory(temp_file.string()));
}

// 测试 getDirectoryContents 函数
TEST_F(FileManagerTest, GetDirectoryContents) {
    // 在临时目录下创建几个文件和子目录
    fs::path file1 = temp_dir / "file1.txt";
    fs::path file2 = temp_dir / "file2.txt";
    fs::path sub_dir = temp_dir / "subdir";
    std::ofstream(file1.string()).put('A');
    std::ofstream(file2.string()).put('B');
    fs::create_directory(sub_dir);

    auto contents = getDirectoryContents(temp_dir.string());
    // 结果中不包含 "." 和 ".."，至少应包含 file1.txt, file2.txt, subdir
    EXPECT_NE(std::find(contents.begin(), contents.end(), "file1.txt"), contents.end());
    EXPECT_NE(std::find(contents.begin(), contents.end(), "file2.txt"), contents.end());
    EXPECT_NE(std::find(contents.begin(), contents.end(), "subdir"), contents.end());
}

// 测试 createFile 与 writeFileContent、readFileContent
TEST_F(FileManagerTest, CreateWriteReadDeleteFile) {
    fs::path test_file = temp_dir / "test.txt";

    // 测试创建文件
    EXPECT_TRUE(createFile(test_file.string()));
    EXPECT_TRUE(fs::exists(test_file));

    // 写入内容到文件
    std::string content = "Line1\nLine2\nLine3";
    EXPECT_TRUE(writeFileContent(test_file.string(), content));

    // 读取指定行，测试读取第二行
    std::string read_content = readFileContent(test_file.string(), 2, 2);
    EXPECT_NE(read_content.find("Line2"), std::string::npos);

    // 删除文件
    EXPECT_TRUE(deleteFileOrDirectory(test_file.string()));
    EXPECT_FALSE(fs::exists(test_file));
}

// 测试 createDirectory 与 deleteFileOrDirectory
TEST_F(FileManagerTest, CreateAndDeleteDirectory) {
    fs::path new_dir = temp_dir / "new_folder";

    // 测试创建目录
    EXPECT_TRUE(createDirectory(new_dir.string()));
    EXPECT_TRUE(fs::exists(new_dir));
    EXPECT_TRUE(fs::is_directory(new_dir));

    // 在目录下创建一个文件
    fs::path inside_file = new_dir / "inside.txt";
    std::ofstream(inside_file.string()) << "Content";

    // 删除目录（包含子文件）
    EXPECT_TRUE(deleteFileOrDirectory(new_dir.string()));
    EXPECT_FALSE(fs::exists(new_dir));
}

// 测试 isForbiddenFile 函数
TEST_F(FileManagerTest, IsForbiddenFile) {
    EXPECT_TRUE(isForbiddenFile("test.o"));
    EXPECT_TRUE(isForbiddenFile("module.dll"));
    EXPECT_FALSE(isForbiddenFile("readme.txt"));
    EXPECT_FALSE(isForbiddenFile("image.png"));
}

// 测试分块读取文件内容缓存功能
TEST_F(FileManagerTest, ReadFileContentCache) {
    fs::path test_file = temp_dir / "cache_test.txt";

    // 构造较长的文本，保证分块读取有效
    std::string content;
    for (int i = 1; i <= 100; ++i) {
        content += "This is line " + std::to_string(i) + "\n";
    }
    EXPECT_TRUE(writeFileContent(test_file.string(), content));

    // 第一次读取，缓存应该为空
    std::string part1 = readFileContent(test_file.string(), 10, 20);
    EXPECT_NE(part1.find("line 10"), std::string::npos);
    EXPECT_NE(part1.find("line 20"), std::string::npos);

    // 等待一小段时间，模拟缓存生效（默认60秒有效，这里无需等待太久）
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 第二次读取，应从缓存中读取
    std::string part2 = readFileContent(test_file.string(), 10, 20);
    EXPECT_EQ(part1, part2);
}
