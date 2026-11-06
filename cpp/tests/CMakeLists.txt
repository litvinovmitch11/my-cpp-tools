function(mycpptools_add_unit_test test_name source library)
    set(target_name "${test_name}_test")

    add_executable("${target_name}" "${source}")
    mycpptools_configure_target("${target_name}")
    target_include_directories("${target_name}" PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries("${target_name}" PRIVATE "${library}")

    add_test(NAME "unit.${test_name}" COMMAND "${target_name}")
    set_tests_properties("unit.${test_name}" PROPERTIES TIMEOUT 10)
endfunction()

mycpptools_add_unit_test(
    directed_graph
    unit/directed_graph_test.cpp
    mycpptools::graph
)
mycpptools_add_unit_test(
    ast_graph
    unit/ast_graph_test.cpp
    mycpptools::ast
)
mycpptools_add_unit_test(
    dot_writer
    unit/dot_writer_test.cpp
    mycpptools::dot
)
mycpptools_add_unit_test(
    graph_dot_serializer
    unit/graph_dot_serializer_test.cpp
    mycpptools::presentation
)

add_custom_target(update-ast-goldens)

function(mycpptools_add_ast_golden_test example_name)
    set(source_file
        "${PROJECT_SOURCE_DIR}/examples/ast/${example_name}.cpp"
    )
    set(expected_file
        "${CMAKE_CURRENT_SOURCE_DIR}/golden/ast/${example_name}.dot"
    )
    set(actual_file
        "${CMAKE_CURRENT_BINARY_DIR}/golden/ast/${example_name}.dot"
    )

    add_test(
        NAME "ast.golden.${example_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DAST_PRINTER=$<TARGET_FILE:AstPrinter>"
            "-DSOURCE_FILE=${source_file}"
            "-DEXPECTED_FILE=${expected_file}"
            "-DACTUAL_FILE=${actual_file}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_ast_golden_test.cmake"
    )
    set_tests_properties("ast.golden.${example_name}" PROPERTIES TIMEOUT 20)

    set(update_target "update-ast-golden-${example_name}")
    add_custom_target("${update_target}"
        COMMAND "${CMAKE_COMMAND}"
            "-DAST_PRINTER=$<TARGET_FILE:AstPrinter>"
            "-DSOURCE_FILE=${source_file}"
            "-DEXPECTED_FILE=${expected_file}"
            "-DACTUAL_FILE=${actual_file}"
            -DUPDATE_GOLDEN=ON
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_ast_golden_test.cmake"
        DEPENDS AstPrinter
        COMMENT "Updating AST golden: ${example_name}"
        VERBATIM
    )
    add_dependencies(update-ast-goldens "${update_target}")
endfunction()

mycpptools_add_ast_golden_test(basic)
mycpptools_add_ast_golden_test(clang_extensions)
mycpptools_add_ast_golden_test(control_flow)
mycpptools_add_ast_golden_test(coroutines)
mycpptools_add_ast_golden_test(empty)
mycpptools_add_ast_golden_test(expressions)
mycpptools_add_ast_golden_test(function_definitions)
mycpptools_add_ast_golden_test(lambdas)
mycpptools_add_ast_golden_test(literals_and_escaping)
mycpptools_add_ast_golden_test(local_classes)
mycpptools_add_ast_golden_test(main_file_only)
mycpptools_add_ast_golden_test(range_for)
mycpptools_add_ast_golden_test(templates_and_methods)
mycpptools_add_ast_golden_test(exceptions_and_labels)

add_test(
    NAME ast.invalid_source
    COMMAND "${CMAKE_COMMAND}"
        "-DAST_PRINTER=$<TARGET_FILE:AstPrinter>"
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ast/invalid.cpp"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_ast_failure_test.cmake"
)
set_tests_properties(ast.invalid_source PROPERTIES TIMEOUT 20)
