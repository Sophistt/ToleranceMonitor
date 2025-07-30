cmake_minimum_required(VERSION 3.12)
project(ToleranceMonitor VERSION 1.0.0 LANGUAGES CXX C)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# 包含头文件目录
include_directories(include)

# 创建 ToleranceChecker 核心库
add_library(ToleranceCheckerCore STATIC
    src/ToleranceChecker.cpp
)
target_link_libraries(ToleranceCheckerCore Threads::Threads)

# 创建 C 接口库
add_library(ToleranceCheckerC STATIC
    src/ToleranceChecker_c.cpp
)
target_link_libraries(ToleranceCheckerC ToleranceCheckerCore Threads::Threads)

# 创建原始的 C++ demo 可执行文件
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} ToleranceCheckerCore)

# 创建 C 接口 demo 可执行文件
add_executable(ToleranceMonitorCDemo src/demo_c.c)
target_link_libraries(ToleranceMonitorCDemo ToleranceCheckerC)

# 链接pthread库
find_package(Threads REQUIRED)

# 设置输出目录
set_target_properties(${PROJECT_NAME} ToleranceMonitorCDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)