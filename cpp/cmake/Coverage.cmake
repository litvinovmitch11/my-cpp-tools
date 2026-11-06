function(mycpptools_add_coverage_target)
    if(NOT MYCPPTOOLS_ENABLE_COVERAGE)
        return()
    endif()

    cmake_parse_arguments(COVERAGE "" "MINIMUM_LINE_PERCENT" "TARGETS" ${ARGN})
    if(NOT COVERAGE_TARGETS)
        message(FATAL_ERROR "C++ coverage requires at least one target")
    endif()
    if(NOT COVERAGE_MINIMUM_LINE_PERCENT)
        set(COVERAGE_MINIMUM_LINE_PERCENT 60)
    endif()

    find_program(LLVM_PROFDATA_EXECUTABLE
        NAMES "llvm-profdata-${LLVM_VERSION_MAJOR}" llvm-profdata
        HINTS "${LLVM_TOOLS_BINARY_DIR}"
        REQUIRED
    )
    find_program(LLVM_COV_EXECUTABLE
        NAMES "llvm-cov-${LLVM_VERSION_MAJOR}" llvm-cov
        HINTS "${LLVM_TOOLS_BINARY_DIR}"
        REQUIRED
    )

    set(coverage_binaries "")
    foreach(target IN LISTS COVERAGE_TARGETS)
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "Unknown C++ coverage target: ${target}")
        endif()
        list(APPEND coverage_binaries "$<TARGET_FILE:${target}>")
    endforeach()
    list(JOIN coverage_binaries "|" coverage_binaries_argument)

    add_custom_target(coverage
        COMMAND "${CMAKE_COMMAND}"
            "-DBUILD_DIRECTORY=${PROJECT_BINARY_DIR}"
            "-DCTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}"
            "-DLLVM_PROFDATA_EXECUTABLE=${LLVM_PROFDATA_EXECUTABLE}"
            "-DLLVM_COV_EXECUTABLE=${LLVM_COV_EXECUTABLE}"
            "-DCOVERAGE_BINARIES=${coverage_binaries_argument}"
            "-DMINIMUM_LINE_PERCENT=${COVERAGE_MINIMUM_LINE_PERCENT}"
            -P "${PROJECT_SOURCE_DIR}/cmake/RunCoverage.cmake"
        DEPENDS ${COVERAGE_TARGETS}
        COMMENT "Running C++ tests and enforcing line coverage"
        USES_TERMINAL
        VERBATIM
    )
endfunction()
