function(mycpptools_add_developer_targets)
    file(GLOB_RECURSE mycpptools_format_files CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/include/*.cpp"
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_SOURCE_DIR}/include/*.hpp"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/*.h"
        "${PROJECT_SOURCE_DIR}/src/*.hpp"
        "${PROJECT_SOURCE_DIR}/examples/*.cpp"
        "${PROJECT_SOURCE_DIR}/examples/*.h"
        "${PROJECT_SOURCE_DIR}/examples/*.hpp"
        "${PROJECT_SOURCE_DIR}/tests/*.cpp"
        "${PROJECT_SOURCE_DIR}/tests/*.h"
        "${PROJECT_SOURCE_DIR}/tests/*.hpp"
    )

    find_program(CLANG_FORMAT_EXECUTABLE
        NAMES clang-format-22 clang-format-21 clang-format
        NO_CACHE
    )
    if(CLANG_FORMAT_EXECUTABLE)
        add_custom_target(format
            COMMAND "${CLANG_FORMAT_EXECUTABLE}"
                -i
                "--style=file:${PROJECT_SOURCE_DIR}/.clang-format"
                ${mycpptools_format_files}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMENT "Formatting C++ sources with clang-format"
            VERBATIM
        )
        add_custom_target(format-check
            COMMAND "${CLANG_FORMAT_EXECUTABLE}"
                --dry-run
                --Werror
                "--style=file:${PROJECT_SOURCE_DIR}/.clang-format"
                ${mycpptools_format_files}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMENT "Checking C++ formatting"
            VERBATIM
        )
        message(STATUS "Using clang-format: ${CLANG_FORMAT_EXECUTABLE}")
    else()
        message(STATUS "clang-format not found; format targets are disabled")
    endif()

    find_program(CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy-22 clang-tidy-21 clang-tidy
        NO_CACHE
    )
    find_program(RUN_CLANG_TIDY_EXECUTABLE
        NAMES run-clang-tidy-22 run-clang-tidy-21 run-clang-tidy
        NO_CACHE
    )
    if(CLANG_TIDY_EXECUTABLE AND RUN_CLANG_TIDY_EXECUTABLE)
        add_custom_target(tidy
            COMMAND "${RUN_CLANG_TIDY_EXECUTABLE}"
                -clang-tidy-binary "${CLANG_TIDY_EXECUTABLE}"
                -config-file "${PROJECT_SOURCE_DIR}/.clang-tidy"
                -p "${PROJECT_BINARY_DIR}"
                -quiet
                -warnings-as-errors "*"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMENT "Running clang-tidy"
            USES_TERMINAL
            VERBATIM
        )
        message(STATUS "Using clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
    else()
        message(STATUS "clang-tidy not found; tidy target is disabled")
    endif()
endfunction()
