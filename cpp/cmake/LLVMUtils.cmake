function(mycpptools_configure_target target)
    separate_arguments(llvm_compile_options NATIVE_COMMAND "${LLVM_DEFINITIONS}")

    target_include_directories(${target} SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
    )
    target_compile_options(${target} PRIVATE ${llvm_compile_options})
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
    target_link_libraries(${target} PRIVATE
        clangAST
        clangBasic
        clangFrontend
        clangTooling
    )
endfunction()
