foreach(required_variable
        BUILD_DIRECTORY
        CTEST_EXECUTABLE
        LLVM_PROFDATA_EXECUTABLE
        LLVM_COV_EXECUTABLE
        COVERAGE_BINARIES
        MINIMUM_LINE_PERCENT)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

set(coverage_directory "${BUILD_DIRECTORY}/coverage")
set(profile_directory "${coverage_directory}/profiles")
set(merged_profile "${coverage_directory}/coverage.profdata")
set(lcov_file "${coverage_directory}/lcov.info")

file(REMOVE_RECURSE "${coverage_directory}")
file(MAKE_DIRECTORY "${profile_directory}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "LLVM_PROFILE_FILE=${profile_directory}/%p-%m.profraw"
        "${CTEST_EXECUTABLE}"
        --test-dir "${BUILD_DIRECTORY}"
        --output-on-failure
    RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "C++ tests failed while collecting coverage")
endif()

file(GLOB raw_profiles "${profile_directory}/*.profraw")
if(NOT raw_profiles)
    message(FATAL_ERROR "C++ tests produced no coverage profiles")
endif()

execute_process(
    COMMAND "${LLVM_PROFDATA_EXECUTABLE}" merge
        -sparse
        ${raw_profiles}
        -o "${merged_profile}"
    RESULT_VARIABLE merge_result
    ERROR_VARIABLE merge_diagnostics
)
if(NOT merge_result EQUAL 0)
    message(FATAL_ERROR "Failed to merge C++ coverage profiles:\n${merge_diagnostics}")
endif()

string(REPLACE "|" ";" coverage_binaries "${COVERAGE_BINARIES}")
list(POP_FRONT coverage_binaries primary_binary)
set(additional_objects "")
foreach(binary IN LISTS coverage_binaries)
    list(APPEND additional_objects "-object=${binary}")
endforeach()

execute_process(
    COMMAND "${LLVM_COV_EXECUTABLE}" export
        "${primary_binary}"
        ${additional_objects}
        "-instr-profile=${merged_profile}"
        -format=lcov
        "-ignore-filename-regex=(/tests/|/examples/|/build[^/]*/|/usr/)"
    RESULT_VARIABLE export_result
    ERROR_VARIABLE export_diagnostics
    OUTPUT_FILE "${lcov_file}"
)
if(NOT export_result EQUAL 0)
    message(FATAL_ERROR "Failed to export C++ coverage:\n${export_diagnostics}")
endif()

file(STRINGS "${lcov_file}" line_totals REGEX "^LF:[0-9]+$")
file(STRINGS "${lcov_file}" line_hits REGEX "^LH:[0-9]+$")
set(total_lines 0)
set(covered_lines 0)
foreach(entry IN LISTS line_totals)
    string(REPLACE "LF:" "" count "${entry}")
    math(EXPR total_lines "${total_lines} + ${count}")
endforeach()
foreach(entry IN LISTS line_hits)
    string(REPLACE "LH:" "" count "${entry}")
    math(EXPR covered_lines "${covered_lines} + ${count}")
endforeach()

if(total_lines EQUAL 0)
    message(FATAL_ERROR "C++ coverage report contains no project source lines")
endif()

math(EXPR coverage_tenths "${covered_lines} * 1000 / ${total_lines}")
math(EXPR coverage_whole "${coverage_tenths} / 10")
math(EXPR coverage_fraction "${coverage_tenths} % 10")
math(EXPR covered_scaled "${covered_lines} * 100")
math(EXPR required_scaled "${total_lines} * ${MINIMUM_LINE_PERCENT}")

message(STATUS
    "C++ line coverage: ${coverage_whole}.${coverage_fraction}% "
    "(${covered_lines}/${total_lines})"
)
if(covered_scaled LESS required_scaled)
    message(FATAL_ERROR
        "C++ line coverage is below ${MINIMUM_LINE_PERCENT}%"
    )
endif()
