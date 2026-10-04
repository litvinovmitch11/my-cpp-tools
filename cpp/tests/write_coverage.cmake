file(GLOB profile_files "${PROFILE_DIR}/*.profraw")
if(NOT profile_files)
    message(FATAL_ERROR "No C++ coverage profiles were produced")
endif()

set(profile_data "${OUTPUT_DIR}/coverage.profdata")
execute_process(
    COMMAND "${LLVM_PROFDATA}" merge -sparse ${profile_files} -o "${profile_data}"
    RESULT_VARIABLE merge_result
    ERROR_VARIABLE merge_error
)
if(NOT merge_result EQUAL 0)
    message(FATAL_ERROR "llvm-profdata failed:\n${merge_error}")
endif()

execute_process(
    COMMAND "${LLVM_COV}" report "${AST_PRINTER}"
        "-instr-profile=${profile_data}"
        -ignore-filename-regex=/usr/
    RESULT_VARIABLE report_result
    OUTPUT_VARIABLE report
    ERROR_VARIABLE report_error
)
if(NOT report_result EQUAL 0)
    message(FATAL_ERROR "llvm-cov report failed:\n${report_error}")
endif()
message("${report}")

string(REGEX MATCH
    "TOTAL[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+[0-9.]+%[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+[0-9.]+%[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+([0-9.]+)%"
    total_match
    "${report}"
)
if(NOT total_match)
    message(FATAL_ERROR "Could not read total line coverage from llvm-cov output")
endif()
set(line_coverage "${CMAKE_MATCH_1}")
if(line_coverage LESS 60)
    message(FATAL_ERROR "C++ line coverage ${line_coverage}% is below 60%")
endif()

execute_process(
    COMMAND "${LLVM_COV}" export "${AST_PRINTER}"
        "-instr-profile=${profile_data}"
        -ignore-filename-regex=/usr/
        -format=lcov
    RESULT_VARIABLE export_result
    OUTPUT_FILE "${OUTPUT_DIR}/lcov.info"
    ERROR_VARIABLE export_error
)
if(NOT export_result EQUAL 0)
    message(FATAL_ERROR "llvm-cov export failed:\n${export_error}")
endif()
