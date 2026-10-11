cmake_minimum_required(VERSION 3.28.3)

project(my-cpp-tools LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MYCPPTOOLS_ENABLE_COVERAGE "Instrument C++ targets for coverage" OFF)

if(MYCPPTOOLS_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "C++ coverage requires Clang")
    endif()

    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include(DeveloperTools)
include(LLVMUtils)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

add_library(mycpptools_graph STATIC
    src/graph/graph.cpp
)
add_library(mycpptools::graph ALIAS mycpptools_graph)
mycpptools_configure_target(mycpptools_graph)

add_library(mycpptools_ast STATIC
    src/ast/ast.cpp
)
add_library(mycpptools::ast ALIAS mycpptools_ast)
mycpptools_configure_target(mycpptools_ast)
target_link_libraries(mycpptools_ast PUBLIC
    mycpptools::graph
)

add_library(mycpptools_clang_ast STATIC
    src/clang/ast_graph_builder.cpp
)
add_library(mycpptools::clang_ast ALIAS mycpptools_clang_ast)
mycpptools_configure_target(mycpptools_clang_ast)
mycpptools_configure_clang_target(mycpptools_clang_ast)
target_link_libraries(mycpptools_clang_ast
    PUBLIC
        mycpptools::ast
        clangTooling
    PRIVATE
        clangAST
        clangBasic
        clangFrontend
)

add_library(mycpptools_cfg STATIC
    src/cfg/cfg_builder.cpp
)
add_library(mycpptools::cfg ALIAS mycpptools_cfg)
mycpptools_configure_target(mycpptools_cfg)
target_link_libraries(mycpptools_cfg PUBLIC
    mycpptools::ast
)

add_library(mycpptools_cdg STATIC
    src/cdg/cdg_builder.cpp
)
add_library(mycpptools::cdg ALIAS mycpptools_cdg)
mycpptools_configure_target(mycpptools_cdg)
target_link_libraries(mycpptools_cdg PUBLIC
    mycpptools::cfg
)

add_library(mycpptools_dot STATIC
    src/dot/dot_writer.cpp
)
add_library(mycpptools::dot ALIAS mycpptools_dot)
mycpptools_configure_target(mycpptools_dot)
target_link_libraries(mycpptools_dot PUBLIC
    mycpptools::graph
)

add_subdirectory(src/tools/AstPrinter)
add_subdirectory(src/tools/CfgPrinter)
add_subdirectory(src/tools/CdgPrinter)

mycpptools_add_developer_targets()
