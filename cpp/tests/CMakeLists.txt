function(mycpptools_add_unit_test test_name source library)
    set(target_name "${test_name}_test")

    add_executable("${target_name}" "${source}")
    mycpptools_configure_target("${target_name}")
    target_include_directories("${target_name}" PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries("${target_name}" PRIVATE "${library}")

    add_test(NAME "unit.${test_name}" COMMAND "${target_name}")
    set_tests_properties("unit.${test_name}" PROPERTIES
        TIMEOUT 10
        LABELS "unit;contract"
    )
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
    cfg_graph
    unit/cfg_graph_test.cpp
    mycpptools::cfg
)
mycpptools_add_unit_test(
    cfg_builder
    unit/cfg_builder_test.cpp
    mycpptools::cfg
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
add_custom_target(update-cfg-goldens)

set(MYCPPTOOLS_EXAMPLES
    basic
    branches
    clang_extensions
    constexpr
    control_flow
    coroutines
    empty
    exceptions_and_labels
    expressions
    function_definitions
    lambdas
    literals_and_escaping
    local_classes
    loops
    main_file_only
    range_for
    switch_and_goto
    templates_and_methods
    unreachable
)

function(mycpptools_add_ast_golden_test example_name)
    set(source_file
        "${PROJECT_SOURCE_DIR}/examples/${example_name}.cpp"
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
    set_tests_properties("ast.golden.${example_name}" PROPERTIES
        TIMEOUT 20
        LABELS "integration;golden;ast"
    )

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

function(mycpptools_add_cfg_golden_test example_name)
    set(source_file
        "${PROJECT_SOURCE_DIR}/examples/${example_name}.cpp"
    )
    set(expected_file
        "${CMAKE_CURRENT_SOURCE_DIR}/golden/cfg/${example_name}.dot"
    )
    set(actual_file
        "${CMAKE_CURRENT_BINARY_DIR}/golden/cfg/${example_name}.dot"
    )

    add_test(
        NAME "cfg.golden.${example_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCFG_PRINTER=$<TARGET_FILE:CfgPrinter>"
            "-DSOURCE_FILE=${source_file}"
            "-DEXPECTED_FILE=${expected_file}"
            "-DACTUAL_FILE=${actual_file}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cfg_golden_test.cmake"
    )
    set_tests_properties("cfg.golden.${example_name}" PROPERTIES
        TIMEOUT 20
        LABELS "integration;golden;cfg"
    )

    set(update_target "update-cfg-golden-${example_name}")
    add_custom_target("${update_target}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCFG_PRINTER=$<TARGET_FILE:CfgPrinter>"
            "-DSOURCE_FILE=${source_file}"
            "-DEXPECTED_FILE=${expected_file}"
            "-DACTUAL_FILE=${actual_file}"
            -DUPDATE_GOLDEN=ON
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cfg_golden_test.cmake"
        DEPENDS CfgPrinter
        COMMENT "Updating CFG golden: ${example_name}"
        VERBATIM
    )
    add_dependencies(update-cfg-goldens "${update_target}")
endfunction()

foreach(example_name IN LISTS MYCPPTOOLS_EXAMPLES)
    mycpptools_add_ast_golden_test("${example_name}")
    mycpptools_add_cfg_golden_test("${example_name}")
endforeach()

add_test(
    NAME cfg.invalid_source
    COMMAND "${CMAKE_COMMAND}"
        "-DCFG_PRINTER=$<TARGET_FILE:CfgPrinter>"
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ast/invalid.cpp"
        "-DEXPECTED_DIAGNOSTIC=error:"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cfg_failure_test.cmake"
)
set_tests_properties(cfg.invalid_source PROPERTIES
    TIMEOUT 20
    LABELS "integration;negative;cfg"
)

add_test(
    NAME ast.invalid_source
    COMMAND "${CMAKE_COMMAND}"
        "-DAST_PRINTER=$<TARGET_FILE:AstPrinter>"
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ast/invalid.cpp"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_ast_failure_test.cmake"
)
set_tests_properties(ast.invalid_source PROPERTIES
    TIMEOUT 20
    LABELS "integration;negative;ast"
)

add_test(
    NAME architecture.compiler_boundary
    COMMAND "${CMAKE_COMMAND}"
        "-DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_compiler_boundary.cmake"
)
set_tests_properties(architecture.compiler_boundary PROPERTIES
    TIMEOUT 10
    LABELS "architecture;contract"
)

add_test(
    NAME architecture.ast_coverage_matrix
    COMMAND "${CMAKE_COMMAND}"
        "-DREPOSITORY_ROOT=${PROJECT_SOURCE_DIR}/.."
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_ast_coverage_matrix.cmake"
)
set_tests_properties(architecture.ast_coverage_matrix PROPERTIES
    TIMEOUT 10
    LABELS "architecture;contract"
)
