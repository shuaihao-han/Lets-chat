cmake_minimum_required(VERSION 3.0)
project(chat) #工程名

# 配置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置 头文件 搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)   # server和client共用的头文件
include_directories(${PROJECT_SOURCE_DIR}/include/server)   # server专有的头文件
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)   # 数据库头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)   # 数据库头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)   # 数据库头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)


# 加载子目录 src 既然进去, 就有 CMakeLists.txt
add_subdirectory(src)
