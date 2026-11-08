foreach(required_variable CFG_PRINTER SOURCE_FILE EXPECTED_FILE ACTUAL_FILE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

get_filename_component(actual_directory "${ACTUAL_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${actual_directory}")

execute_process(
    COMMAND "${CFG_PRINTER}" "${SOURCE_FILE}" -- -std=c++20
    RESULT_VARIABLE cfg_printer_result
    ERROR_VARIABLE cfg_printer_diagnostics
    OUTPUT_FILE "${ACTUAL_FILE}"
    TIMEOUT 15
)

if(NOT cfg_printer_result EQUAL 0)
    message(FATAL_ERROR
        "CfgPrinter failed for ${SOURCE_FILE}\n${cfg_printer_diagnostics}"
    )
endif()

if(NOT cfg_printer_diagnostics STREQUAL "")
    message(FATAL_ERROR
        "CfgPrinter emitted diagnostics for ${SOURCE_FILE}\n"
        "${cfg_printer_diagnostics}"
    )
endif()

if(UPDATE_GOLDEN)
    get_filename_component(expected_directory "${EXPECTED_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${expected_directory}")
    file(COPY_FILE "${ACTUAL_FILE}" "${EXPECTED_FILE}" ONLY_IF_DIFFERENT)
    return()
endif()

if(NOT EXISTS "${EXPECTED_FILE}")
    message(FATAL_ERROR
        "Missing CFG golden: ${EXPECTED_FILE}\n"
        "Run the update-cfg-goldens target after reviewing the output."
    )
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files
        "${EXPECTED_FILE}" "${ACTUAL_FILE}"
    RESULT_VARIABLE comparison_result
)

if(NOT comparison_result EQUAL 0)
    message(FATAL_ERROR
        "CFG output differs from its golden.\n"
        "Expected: ${EXPECTED_FILE}\n"
        "Actual:   ${ACTUAL_FILE}\n"
        "Review the diff before running the update-cfg-goldens target."
    )
endif()
