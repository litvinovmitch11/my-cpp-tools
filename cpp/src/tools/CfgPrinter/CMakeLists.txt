add_executable(CfgPrinter
    main.cpp
)

mycpptools_configure_target(CfgPrinter)
mycpptools_configure_clang_target(CfgPrinter)

target_link_libraries(CfgPrinter PRIVATE
    mycpptools::clang_ast
    mycpptools::cfg
    mycpptools::dot
)
