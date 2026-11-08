foreach(required_variable CFG_PRINTER SOURCE_FILE EXPECTED_DIAGNOSTIC)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${CFG_PRINTER}" "${SOURCE_FILE}" -- -std=c++20
    RESULT_VARIABLE cfg_printer_result
    OUTPUT_VARIABLE cfg_printer_output
    ERROR_VARIABLE cfg_printer_diagnostics
    TIMEOUT 15
)

if(cfg_printer_result EQUAL 0)
    message(FATAL_ERROR "CfgPrinter accepted invalid source code")
endif()

if(NOT cfg_printer_output STREQUAL "")
    message(FATAL_ERROR "CfgPrinter wrote a partial graph after failure")
endif()

if(NOT cfg_printer_diagnostics MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR
        "CfgPrinter diagnostic did not contain '${EXPECTED_DIAGNOSTIC}'\n"
        "${cfg_printer_diagnostics}"
    )
endif()
