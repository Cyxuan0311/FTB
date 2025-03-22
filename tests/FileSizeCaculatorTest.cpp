#include<gtest/gtest.h>
#include "../include/FileSizeCalculator.hpp"
#include "../include/FileManager.hpp"

#include <filesystem>
#include <atomic>
#include <fstream>

namespace fs = std::filesystem;

class FileSizeCalculatorTest : public ::testing::Test {
    protected:
        std::string testDir;

        void createFile(const std::string &path,size_t size){
            std::ofstream file(path,std::ios::binary);
            file.seekp(size-1);
            file.put(0);
            file.close();
        }

        void SetUp() override{
            //创建测试目录
            testDir="test_directory";
            fs::create_directory(testDir);

            createFile(testDir+"/file1.txt",1024);
            createFile(testDir+"/file2.txt",2048);
            createFile(testDir+"/file3.txt",512);
        }

        void TearDown() override{
            fs::remove_all(testDir);
        }
};

// 测试文件夹大小
TEST_F(FileSizeCalculatorTest,CalculateSizes_CorrectTotalSize){
    std::atomic<uintmax_t> total_size(0);
    std::atomic<double> size_ratio(0.0);
    std::string selected_size;

    FileSizeCalculator::CalculateSizes(testDir,-1,total_size,size_ratio,selected_size);

    //std::cout << "Expected: " << (1024+2048+512) << ", Actual: " << total_size.load() << std::endl;

    EXPECT_EQ(total_size.load(),1024+2048+512);
}

// 测试单个文件大小计算
TEST_F(FileSizeCalculatorTest,CalculateSizes_CorrectFileSize){
    std::atomic<uintmax_t> total_size(0);
    std::atomic<double> size_ratio(0.0);
    std::string selected_size;

    FileSizeCalculator::CalculateSizes(testDir,1,total_size,size_ratio,selected_size);

    EXPECT_EQ(selected_size,"2.00 KB");
}

// 测试文件比例计算
TEST_F(FileSizeCalculatorTest,CalculateSizes_CorrectRatio){
    std::atomic<uintmax_t> total_size(0);
    std::atomic<double> size_ratio(0.0);
    std::string selected_size;

    FileSizeCalculator::CalculateSizes(testDir,0,total_size,size_ratio,selected_size);

    double expected_ratio=1024.0/(1024+2048+512);
    EXPECT_NEAR(size_ratio.load(),expected_ratio,0.01);
}

// 测试空目录
TEST_F(FileSizeCalculatorTest,CalculateSizes_EmptyDirectory){
    std::string emptyDir=testDir+"/empty";
    fs::create_directory(emptyDir);

    std::atomic<uintmax_t> total_size(0);
    std::atomic<double> size_ratio(0.0);
    std::string selected_size;

    FileSizeCalculator::CalculateSizes(emptyDir,0,total_size,size_ratio,selected_size);

    EXPECT_EQ(total_size.load(),0);
    EXPECT_EQ(selected_size,"0 B");

    fs::remove_all(emptyDir);
}

// 测试选中文件索引超出范围
TEST_F(FileSizeCalculatorTest,CalculateSizes_Out_range_Index){
    std::atomic<uintmax_t> total_size(0);
    std::atomic<double> size_ratio(0.0);
    std::string selected_size;

    FileSizeCalculator::CalculateSizes(testDir,10,total_size,size_ratio,selected_size);

    EXPECT_EQ(selected_size,"0 B");
    EXPECT_EQ(size_ratio.load(),0.0);
}
