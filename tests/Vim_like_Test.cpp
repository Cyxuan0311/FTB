#include "gtest/gtest.h"
#include "../include/FTB/Vim/Vim_Like.hpp"
#include <ftxui/component/event.hpp>
#include <vector>
#include <string>

using namespace ftxui;

// 测试保存修改后的内容：删除字符后退出时返回修改后的文本
TEST(VimLikeEditorTest, SaveModifiedContent) {
    VimLikeEditor editor;
    std::vector<std::string> exit_content;

    // 初始化文本内容为 "Hello"
    editor.SetContent({"Hello"});
    // 设置退出回调，将返回内容保存到 exit_content
    editor.SetOnExit([&exit_content](const std::vector<std::string>& content) {
         exit_content = content;
    });
    
    // 按 'i' 进入编辑模式
    editor.OnEvent(Event::Character("i"));
    // 向右移动光标1个字符（光标位于索引1处）
    editor.OnEvent(Event::ArrowRight);
    // 按 Backspace 删除光标左侧的字符，此处应删除 'H'
    editor.OnEvent(Event::Backspace);
    // 按 Ctrl+D 保存并退出编辑模式
    editor.OnEvent(Event::CtrlD);
    
    ASSERT_EQ(exit_content.size(), 1);
    EXPECT_EQ(exit_content[0], "ello");
}

// 测试放弃修改：进行修改后使用 Ctrl+F 退出，应返回原始内容
TEST(VimLikeEditorTest, CancelModification) {
    VimLikeEditor editor;
    std::vector<std::string> exit_content;

    // 初始化文本内容为 "Hello"
    editor.SetContent({"Hello"});
    editor.SetOnExit([&exit_content](const std::vector<std::string>& content) {
         exit_content = content;
    });
    
    // 进入编辑模式
    editor.OnEvent(Event::Character("i"));
    // 模拟在文本前插入字符 'X'（此时实际文本变为 "XHello"）
    editor.OnEvent(Event::Character("X"));
    // 使用 Ctrl+F 退出编辑模式，放弃所有修改
    editor.OnEvent(Event::CtrlF);
    
    // 返回的内容应为原始文本 "Hello"
    ASSERT_EQ(exit_content.size(), 1);
    EXPECT_EQ(exit_content[0], "Hello");
}

// 测试换行功能：在一行中间插入换行符，文本拆分为两行
TEST(VimLikeEditorTest, NewLineInsertion) {
    VimLikeEditor editor;
    std::vector<std::string> exit_content;

    // 初始化内容为 "HelloWorld"
    editor.SetContent({"HelloWorld"});
    editor.SetOnExit([&exit_content](const std::vector<std::string>& content) {
         exit_content = content;
    });
    
    // 进入编辑模式
    editor.OnEvent(Event::Character("i"));
    // 将光标向右移动 5 个位置，位于 "Hello" 与 "World" 的分界处
    for (int i = 0; i < 5; ++i) {
        editor.OnEvent(Event::ArrowRight);
    }
    // 按 Return 键分割当前行
    editor.OnEvent(Event::Return);
    // 使用 Ctrl+D 保存退出
    editor.OnEvent(Event::CtrlD);
    
    ASSERT_EQ(exit_content.size(), 2);
    EXPECT_EQ(exit_content[0], "Hello");
    EXPECT_EQ(exit_content[1], "World");
}

// 测试行合并功能：在行首按 Backspace，将当前行与上一行合并
TEST(VimLikeEditorTest, BackspaceLineMerge) {
    VimLikeEditor editor;
    std::vector<std::string> exit_content;

    // 初始化两行内容
    editor.SetContent({"Hello", "World"});
    editor.SetOnExit([&exit_content](const std::vector<std::string>& content) {
         exit_content = content;
    });
    
    // 进入编辑模式
    editor.OnEvent(Event::Character("i"));
    // 移动光标到第二行开头（行索引1，列索引0）
    editor.MoveCursorTo(1, 0);
    // 按 Backspace 键，预期将第二行 "World" 合并到第一行后
    editor.OnEvent(Event::Backspace);
    // 使用 Ctrl+D 保存退出
    editor.OnEvent(Event::CtrlD);
    
    ASSERT_EQ(exit_content.size(), 1);
    EXPECT_EQ(exit_content[0], "HelloWorld");
}
