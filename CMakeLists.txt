# 设置最低的CMake版本
cmake_minimum_required(VERSION 3.10)

# 设置编译器
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

# 设置构建类型为Debug
set(CMAKE_BUILD_TYPE Debug)
# 声明你的项目名称
project(SIMPLEDB)

# 初始化sql-parser
execute_process(
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third_party/sql-parser/src
)

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置库文件的输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置头文件查找路径
set(THIRD_PARTY_INCLUDE_DIR
        ${PROJECT_SOURCE_DIR}/third_party/sql-parser/src
)

include_directories(${THIRD_PARTY_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/src/include)

# 链接第三方库到你的项目
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/third_party/sql-parser)
# 添加子目录，用于构建各个模块或组件
add_subdirectory(src)

# 添加测试目录
add_subdirectory(tests)

# 添加第三方库
add_subdirectory(third_party)


