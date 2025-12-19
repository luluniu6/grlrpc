cmake_minimum_required(VERSION 3.10)
project(GrlRPC)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置编译类型和调试标志
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra")

# 查找 JsonCpp 依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

# 如果 pkg-config 没找到，尝试手动查找 jsoncpp
if(NOT JSONCPP_FOUND)
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES jsoncpp)
    find_library(JSONCPP_LIBRARY jsoncpp
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    
    if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
        set(JSONCPP_FOUND TRUE)
        set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
        set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
    endif()
endif()

if(NOT JSONCPP_FOUND)
    message(FATAL_ERROR "JsonCpp not found. Please install libjsoncpp-dev")
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(/usr/include/jsoncpp)

# 序列化框架库
add_library(grlrpc_serialization STATIC
    src/serialization_framework.cpp
)
target_include_directories(grlrpc_serialization PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
)
target_link_libraries(grlrpc_serialization ${JSONCPP_LIBRARIES})

# 网络库
add_library(grlrpc_network STATIC
    src/network.cpp
)
target_include_directories(grlrpc_network PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# RPC 框架库
add_library(grlrpc_framework STATIC
    src/rpc_framework.cpp
)
target_include_directories(grlrpc_framework PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
)
target_link_libraries(grlrpc_framework 
    grlrpc_serialization 
    grlrpc_network
    ${JSONCPP_LIBRARIES}
)

# 用户类型序列化器库
add_library(grlrpc_user_types STATIC
    src/user_types.cpp
    src/user_type_serializers.cpp
)
target_include_directories(grlrpc_user_types PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
)
target_link_libraries(grlrpc_user_types 
    grlrpc_serialization
    ${JSONCPP_LIBRARIES}
)

# 主可执行文件
add_executable(grlrpc_server src/main.cpp)
target_link_libraries(grlrpc_server 
    grlrpc_framework 
    grlrpc_user_types
    grlrpc_serialization 
    grlrpc_network
    ${JSONCPP_LIBRARIES}
)

# 编译选项
target_compile_options(grlrpc_serialization PRIVATE -Wall -Wextra)
target_compile_options(grlrpc_network PRIVATE -Wall -Wextra)
target_compile_options(grlrpc_framework PRIVATE -Wall -Wextra)
target_compile_options(grlrpc_user_types PRIVATE -Wall -Wextra)

# 测试配置 (可选)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    # 查找 Google Test
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Google Test found")
    else()
        message(STATUS "Google Test not found - tests will be disabled")
    endif()
    
    # 查找 RapidCheck (属性测试)
    find_path(RAPIDCHECK_INCLUDE_DIR rapidcheck.h
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES rapidcheck)
    find_library(RAPIDCHECK_LIBRARY rapidcheck
        PATHS /usr/lib /usr/local/lib)
    
    if(RAPIDCHECK_INCLUDE_DIR AND RAPIDCHECK_LIBRARY)
        set(RAPIDCHECK_FOUND TRUE)
        message(STATUS "RapidCheck found")
    else()
        message(STATUS "RapidCheck not found - property tests will be disabled")
    endif()
endif()

# 类型注册表测试
add_executable(type_registry_test tests/type_registry_test.cpp)
target_include_directories(type_registry_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(type_registry_test PRIVATE -Wall -Wextra)

# 打印配置信息
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "JsonCpp Include: ${JSONCPP_INCLUDE_DIRS}")
message(STATUS "JsonCpp Libraries: ${JSONCPP_LIBRARIES}")
