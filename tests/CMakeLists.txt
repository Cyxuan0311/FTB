# 进入 tests 目录时的 CMakeLists.txt

# 查找 GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 测试源文件列表
set(TEST_SOURCES
    UIManagerTest.cpp
    FileSizeCaculatorTest.cpp
    Vim_like_Test.cpp
    main.cpp
    FileManagerTest.cpp
)

# 构建测试可执行文件
add_executable(FTB_tests ${TEST_SOURCES})

# 链接测试所需库
target_link_libraries(FTB_tests
    PRIVATE gtest
    PRIVATE gtest_main
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE pthread
    PRIVATE FTB_core
)

# 启用测试
enable_testing()
add_test(NAME FTB_tests COMMAND FTB_tests)
