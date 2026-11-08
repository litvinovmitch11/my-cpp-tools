function(mycpptools_configure_target target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF
    )

    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()

function(mycpptools_configure_llvm_target target)
    separate_arguments(llvm_compile_options NATIVE_COMMAND "${LLVM_DEFINITIONS}")

    target_include_directories(${target} SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
    )
    target_compile_options(${target} PRIVATE
        ${llvm_compile_options}
    )
endfunction()

function(mycpptools_configure_clang_target target)
    mycpptools_configure_llvm_target(${target})

    target_include_directories(${target} SYSTEM PRIVATE
        ${CLANG_INCLUDE_DIRS}
    )
endfunction()
