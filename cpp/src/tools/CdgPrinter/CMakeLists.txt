add_executable(CdgPrinter
    main.cpp
)

mycpptools_configure_target(CdgPrinter)
mycpptools_configure_clang_target(CdgPrinter)

target_link_libraries(CdgPrinter PRIVATE
    mycpptools::clang_ast
    mycpptools::cdg
    mycpptools::dot
)
