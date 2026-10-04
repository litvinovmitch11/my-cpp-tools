set(AST_PRINTER_TARGET AstPrinter)
set(AST_TEST_RUNNER "${CMAKE_CURRENT_SOURCE_DIR}/run_ast_printer_test.cmake")
set(AST_TEST_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

function(add_ast_printer_test test_case fixture)
    add_test(
        NAME "ast.${test_case}"
        COMMAND "${CMAKE_COMMAND}"
            "-DAST_PRINTER=$<TARGET_FILE:${AST_PRINTER_TARGET}>"
            "-DSOURCE=${AST_TEST_FIXTURES}/${fixture}"
            "-DTEST_CASE=${test_case}"
            -P "${AST_TEST_RUNNER}"
    )
    set_tests_properties("ast.${test_case}" PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "LLVM_PROFILE_FILE=${PROJECT_BINARY_DIR}/coverage/profiles/%p-%m.profraw"
    )
endfunction()

add_ast_printer_test(valid_simple simple.cpp)
add_ast_printer_test(multiple_functions multiple_functions.cpp)
add_ast_printer_test(invalid_source invalid.cpp)
add_ast_printer_test(dot_escaping escaping.cpp)
add_ast_printer_test(excludes_includes includes.cpp)
add_ast_printer_test(deterministic simple.cpp)

if(MYCPPTOOLS_ENABLE_COVERAGE)
    string(REGEX MATCH "^[0-9]+" LLVM_COMPILER_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(LLVM_PROFDATA_EXECUTABLE
        NAMES "llvm-profdata-${LLVM_COMPILER_MAJOR}" llvm-profdata
        REQUIRED
    )
    find_program(LLVM_COV_EXECUTABLE
        NAMES "llvm-cov-${LLVM_COMPILER_MAJOR}" llvm-cov
        REQUIRED
    )
    add_custom_target(coverage
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${PROJECT_BINARY_DIR}/coverage"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${PROJECT_BINARY_DIR}/coverage/profiles"
        COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
        COMMAND "${CMAKE_COMMAND}"
            "-DAST_PRINTER=$<TARGET_FILE:${AST_PRINTER_TARGET}>"
            "-DPROFILE_DIR=${PROJECT_BINARY_DIR}/coverage/profiles"
            "-DOUTPUT_DIR=${PROJECT_BINARY_DIR}/coverage"
            "-DLLVM_PROFDATA=${LLVM_PROFDATA_EXECUTABLE}"
            "-DLLVM_COV=${LLVM_COV_EXECUTABLE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/write_coverage.cmake"
        DEPENDS ${AST_PRINTER_TARGET}
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        USES_TERMINAL
        VERBATIM
    )
endif()
