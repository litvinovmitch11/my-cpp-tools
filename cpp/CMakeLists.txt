cmake_minimum_required(VERSION 3.28.3)
project(my-cpp-tools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LLVMUtils)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

add_library(mycpptools STATIC
    src/common/utils.cpp
)
target_include_directories(mycpptools PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
mycpptools_configure_target(mycpptools)

add_subdirectory(src/tools/AstPrinter)
add_subdirectory(src/tools/CfgBuilder)
add_subdirectory(src/tools/CdgBuilder)

# enable_testing()
# add_subdirectory(tests)
