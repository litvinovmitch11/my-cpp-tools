add_executable(AstPrinter
    main.cpp
)

mycpptools_configure_target(AstPrinter)
mycpptools_configure_clang_target(AstPrinter)

target_link_libraries(AstPrinter PRIVATE
    mycpptools::clang_ast
    mycpptools::presentation
)
