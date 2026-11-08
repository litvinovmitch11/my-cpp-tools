if(NOT DEFINED PROJECT_SOURCE_DIR)
    message(FATAL_ERROR "PROJECT_SOURCE_DIR is required")
endif()

set(core_modules graph source ast cfg cdg dot presentation)
set(core_files)
foreach(module IN LISTS core_modules)
    file(GLOB_RECURSE module_files LIST_DIRECTORIES FALSE
        "${PROJECT_SOURCE_DIR}/include/mycpptools/${module}/*.hpp"
        "${PROJECT_SOURCE_DIR}/src/${module}/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/${module}/*.hpp"
    )
    list(APPEND core_files ${module_files})
endforeach()

foreach(core_file IN LISTS core_files)
    file(READ "${core_file}" contents)
    if(contents MATCHES "#[ \t]*include[ \t]*[<\"](clang|llvm)/")
        message(FATAL_ERROR
            "Compiler header dependency escaped into core: ${core_file}")
    endif()
    if(contents MATCHES "(^|[^A-Za-z0-9_])(clang|llvm)::")
        message(FATAL_ERROR
            "Compiler type dependency escaped into core: ${core_file}")
    endif()
endforeach()

file(GLOB_RECURSE cfg_files LIST_DIRECTORIES FALSE
    "${PROJECT_SOURCE_DIR}/include/mycpptools/cfg/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/cfg/*.cpp"
)
foreach(cfg_file IN LISTS cfg_files)
    file(READ "${cfg_file}" contents)
    if(contents MATCHES "ConcreteKind")
        message(FATAL_ERROR
            "CFG must not branch on parser class names: ${cfg_file}")
    endif()
endforeach()
