cmake_minimum_required(VERSION 3.0)
project(Logtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
include_directories(../Auxiliary ../ThreadPool ../MemoryPool ../Reactor ../Buffer ../HttpConnection ../LFUCache ../LogSystem ../SqlConnectionPool ../Timer ../Locker)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
# 添加可执行文件
file(GLOB_RECURSE SRC_LIST
    "*.cc"
    "../Auxiliary/*.cc"
    "../MemoryPool/*.cc"
    "../ThreadPool/*.cc"
    "../Reactor/*.cc"
    "../Buffer/*.cc"
    "../HttpConnection/*.cc"
    "../LFUCache/*.cc"
    "../LogSystem/*.cc"
    "../SqlConnectionPool/*.cc"
    "../Timer/*.cc"
    "../Locker/*.cc"
)
add_executable(Logtest ${SRC_LIST})

# 查找 MySQL 库  
find_library(MYSQL_LIBRARIES mysqlclient)  
  
# 如果找到了 MySQL 库，链接到可执行文件  
if(MYSQL_LIBRARIES)  
    target_link_libraries(Logtest ${MYSQL_LIBRARIES})  
else()  
    message(FATAL_ERROR "MySQL library not found")  
endif()