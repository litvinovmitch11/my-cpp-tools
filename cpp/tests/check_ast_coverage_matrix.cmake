if(NOT DEFINED REPOSITORY_ROOT)
    message(FATAL_ERROR "REPOSITORY_ROOT is required")
endif()

set(ast_header
    "${REPOSITORY_ROOT}/cpp/include/mycpptools/ast/ast_graph.hpp")
set(readme "${REPOSITORY_ROOT}/README.md")

file(READ "${ast_header}" ast_contents)
file(READ "${readme}" readme_contents)

string(REGEX MATCH
    "enum class AstNodeKind[ \t\r\n]*\\{([^}]*)\\}"
    ast_node_kind_match
    "${ast_contents}"
)
if(NOT ast_node_kind_match)
    message(FATAL_ERROR "Could not find AstNodeKind in ${ast_header}")
endif()

set(ast_node_kind_body "${CMAKE_MATCH_1}")
string(REGEX MATCHALL "[A-Za-z][A-Za-z0-9_]*" ast_node_kinds
    "${ast_node_kind_body}")

foreach(ast_node_kind IN LISTS ast_node_kinds)
    string(FIND "${readme_contents}" "`${ast_node_kind}`" kind_position)
    if(kind_position EQUAL -1)
        message(FATAL_ERROR
            "README AST coverage matrix does not classify ${ast_node_kind}")
    endif()
endforeach()
