foreach(required_variable AST_PRINTER SOURCE_FILE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${AST_PRINTER}" "${SOURCE_FILE}" -- -std=c++20
    RESULT_VARIABLE ast_printer_result
    OUTPUT_VARIABLE ast_printer_output
    ERROR_VARIABLE ast_printer_diagnostics
    TIMEOUT 15
)

if(NOT ast_printer_result EQUAL 2)
    message(FATAL_ERROR
        "AstPrinter returned ${ast_printer_result}; expected invalid-source exit code 2"
    )
endif()

if(NOT ast_printer_output STREQUAL "")
    message(FATAL_ERROR "AstPrinter wrote a partial graph for invalid source")
endif()

if(ast_printer_diagnostics STREQUAL "")
    message(FATAL_ERROR "AstPrinter did not explain the invalid source")
endif()
