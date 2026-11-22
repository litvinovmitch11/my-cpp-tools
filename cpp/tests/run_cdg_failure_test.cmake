foreach(required_variable CDG_PRINTER SOURCE_FILE EXPECTED_DIAGNOSTIC)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${CDG_PRINTER}" "${SOURCE_FILE}" -- -std=c++20
    RESULT_VARIABLE cdg_printer_result
    OUTPUT_VARIABLE cdg_printer_output
    ERROR_VARIABLE cdg_printer_diagnostics
    TIMEOUT 15
)

if(NOT cdg_printer_result EQUAL 2)
    message(FATAL_ERROR
        "CdgPrinter returned ${cdg_printer_result}; expected invalid-source exit code 2"
    )
endif()

if(NOT cdg_printer_output STREQUAL "")
    message(FATAL_ERROR "CdgPrinter wrote a partial graph after failure")
endif()

if(NOT cdg_printer_diagnostics MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR
        "CdgPrinter diagnostic did not contain '${EXPECTED_DIAGNOSTIC}'\n"
        "${cdg_printer_diagnostics}"
    )
endif()
