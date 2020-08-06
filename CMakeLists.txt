cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_FLAGS "-O0")
set(CMAKE_CXX_FLAGS "-O0")
if(APPLE)
    set(CMAKE_C_COMPILER "/usr/local/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
else()
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
endif()
project(yapl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

if(APPLE)
    set(LLVM_DIR ${CMAKE_SOURCE_DIR}/llvm-libs/cmake/llvm)
else()
    set(LLVM_DIR /usr/local/lib64/cmake/llvm)
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmaike in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(${LLVM_DEFINITIONS})

add_executable(
        yapl
        main.cpp)


llvm_map_components_to_libnames(llvm_libs core orcjit mcjit native)

add_subdirectory(lib)

target_link_libraries(yapl PRIVATE irgenerator)

