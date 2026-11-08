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
include(Coverage)
include(LLVMUtils)
include(CTest)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

add_library(mycpptools_graph STATIC
    src/graph/directed_graph.cpp
)
add_library(mycpptools::graph ALIAS mycpptools_graph)
mycpptools_configure_target(mycpptools_graph)

add_library(mycpptools_source STATIC
    src/source/function_info.cpp
)
add_library(mycpptools::source ALIAS mycpptools_source)
mycpptools_configure_target(mycpptools_source)

add_library(mycpptools_ast STATIC
    src/ast/ast_graph.cpp
)
add_library(mycpptools::ast ALIAS mycpptools_ast)
mycpptools_configure_target(mycpptools_ast)
target_link_libraries(mycpptools_ast PUBLIC
    mycpptools::graph
    mycpptools::source
)

add_library(mycpptools_clang_ast STATIC
    src/clang/ast_graph_builder.cpp
    src/clang/clang_ast_classifier.cpp
    src/clang/clang_ast_mapper.cpp
    src/clang/function_ast_graph_builder.cpp
)
add_library(mycpptools::clang_ast ALIAS mycpptools_clang_ast)
mycpptools_configure_target(mycpptools_clang_ast)
mycpptools_configure_clang_target(mycpptools_clang_ast)
target_link_libraries(mycpptools_clang_ast
    PUBLIC
        mycpptools::ast
    PRIVATE
        clangTooling
        clangAST
        clangBasic
        clangFrontend
        clangIndex
        clangLex
)

add_library(mycpptools_cfg STATIC
    src/cfg/cfg_builder.cpp
    src/cfg/cfg_graph.cpp
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

add_library(mycpptools_dot STATIC
    src/dot/dot_writer.cpp
)
add_library(mycpptools::dot ALIAS mycpptools_dot)
mycpptools_configure_target(mycpptools_dot)

add_library(mycpptools_presentation STATIC
    src/presentation/graph_dot_serializer.cpp
)
add_library(mycpptools::presentation ALIAS mycpptools_presentation)
mycpptools_configure_target(mycpptools_presentation)
target_link_libraries(mycpptools_presentation
    PUBLIC
        mycpptools::ast
        mycpptools::cfg
    PRIVATE
        mycpptools::dot
)

add_subdirectory(src/tools/AstPrinter)
add_subdirectory(src/tools/CfgPrinter)
add_subdirectory(src/tools/CdgPrinter)

if(BUILD_TESTING)
    add_subdirectory(tests)

    mycpptools_add_coverage_target(
        TARGETS
            AstPrinter
            CfgPrinter
            directed_graph_test
            ast_graph_test
            cfg_graph_test
            cfg_builder_test
            dot_writer_test
            graph_dot_serializer_test
        MINIMUM_LINE_PERCENT 60
    )
elseif(MYCPPTOOLS_ENABLE_COVERAGE)
    message(FATAL_ERROR "C++ coverage requires BUILD_TESTING=ON")
endif()

mycpptools_add_developer_targets()
