function(mycpptools_configure_target target)
    target_include_directories(${target} SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
    )
    target_compile_definitions(${target} PRIVATE ${LLVM_DEFINITIONS})
    target_link_libraries(${target} PRIVATE
    )
endfunction()
