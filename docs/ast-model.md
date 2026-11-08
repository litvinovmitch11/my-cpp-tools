# AST graph model and coverage

This document describes the project-owned AST graph produced by `AstPrinter`.
It is both a user-facing description of the current output and a contributor
checklist for future AST changes.

The model is intentionally smaller and more stable than Clang's complete AST.
Clang parses C++ and supplies semantic information, while `my-cpp-tools` copies
the relevant data into an owning graph that does not retain compiler objects.

The status vocabulary shared by all analysis stages is documented in
[Analysis support and guarantees](analysis-support.md).

## Pipeline and ownership

```text
Clang AST
    -> Clang adapter and classifier
    -> FunctionAstGraph objects
    -> GraphDotSerializer
    -> DOT
```

Each collected function definition produces one `FunctionAstGraph`. A completed
graph owns its node labels, metadata, edges, function identity, and source
offsets. It contains no `clang::Decl`, `clang::Stmt`, `ASTContext`, or borrowed
source-manager state.

A function graph is a rooted ordered tree:

- the root is the function declaration;
- every other node has exactly one parent;
- every node is reachable from the root;
- edges preserve Clang traversal order through contiguous child ordinals;
- cycles and disconnected nodes are rejected when the graph is constructed.

The collection operation is transactional. A successful tool run replaces the
caller's graph collection. A failed run does not publish partial graphs.

## Which functions are collected

Only user-written definitions whose expansion location is in the main input
file are collected.

Included by the model:

- free functions and namespace functions;
- member functions, constructors, destructors, and overloaded operators;
- primary function-template definitions and explicit specializations;
- explicitly defaulted and deleted functions;
- methods of local classes, as independent function graphs.

Excluded by design:

- declarations without a definition;
- definitions originating only from an included header;
- compiler-generated functions and implicit template instantiations;
- duplicate function identities encountered during one tool run;
- lambda call operators as independent graphs.

A lambda expression and its body remain part of the enclosing function graph.
Likewise, a local class declaration remains visible in the enclosing function,
but its method declarations and bodies occur only in their own graphs.

## Node kinds

Every node stores a stable project category in `AstNodeKind` and the exact
Clang class name in `AstNodeMetadata::ConcreteKind`. Downstream analysis must
use `AstNodeKind`; the concrete Clang name is diagnostic and presentation data.
Unlisted Clang nodes are retained under an `Other...` category rather than
dropped.

### Declarations

| Project kind | Clang nodes represented |
| --- | --- |
| `FunctionDeclaration` | `FunctionDecl` and derived function declarations, including methods and constructors |
| `ParameterDeclaration` | `ParmVarDecl` |
| `VariableDeclaration` | non-parameter `VarDecl` nodes |
| `FieldDeclaration` | `FieldDecl` |
| `RecordDeclaration` | `RecordDecl`, including C++ record declarations |
| `OtherDeclaration` | every other traversed declaration, such as access specifiers and bindings |

Implicit declarations are not materialized as nodes, but traversal continues
through their relevant descendants. Template parameter declarations belong to
the surrounding template declaration in Clang and are not currently copied
into a function graph.

### Statements

| Project kind | C++ or Clang construct |
| --- | --- |
| `CompoundStatement` | compound body (`{ ... }`) |
| `DeclarationStatement` | declaration used as a statement |
| `IfStatement` | runtime `if` |
| `ConstexprIfStatement` | `if constexpr` |
| `SwitchStatement` | `switch` |
| `CaseStatement` | `case` |
| `DefaultStatement` | `default` |
| `ForStatement` | classic `for` |
| `RangeForStatement` | range-based `for` |
| `WhileStatement` | `while` |
| `DoStatement` | `do` / `while` |
| `ReturnStatement` | `return` |
| `BreakStatement` | `break` |
| `ContinueStatement` | `continue` |
| `GotoStatement` | direct `goto` |
| `IndirectGotoStatement` | Clang indirect-goto statement |
| `LabelStatement` | a labeled statement |
| `TryStatement` | C++ `try` |
| `CatchStatement` | C++ `catch` |
| `AsmStatement` | inline assembly statement |
| `CoroutineBodyStatement` | Clang's materialized coroutine body |
| `CoReturnStatement` | `co_return` |
| `OtherStatement` | any other traversed non-expression statement |

### Expressions

| Project kind | C++ or Clang construct |
| --- | --- |
| `LambdaExpression` | lambda expression |
| `ThrowExpression` | `throw` expression |
| `CoAwaitExpression` | `co_await` |
| `CoYieldExpression` | `co_yield` |
| `CallExpression` | function and member calls, including overloaded operator calls represented by Clang as calls |
| `BinaryExpression` | built-in binary operator |
| `UnaryExpression` | built-in unary operator |
| `ConditionalExpression` | `condition ? left : right` and GNU `left ?: right` |
| `CompileTimeConditionalExpression` | GNU `__builtin_choose_expr` |
| `DeclarationReferenceExpression` | reference to a declaration |
| `AddressOfLabelExpression` | address of a source label used by indirect goto |
| `MemberExpression` | member access |
| `CastExpression` | explicit and implicit casts |
| `LiteralExpression` | integer, floating-point, character, string, Boolean, and null-pointer literals |
| `OtherExpression` | every other expression, including construction, `this`, subscripting, and initializer-list nodes |

Implicit expressions are retained. For example, an implicit conversion may
appear as an `OtherExpression` or `CastExpression` node even though no token was
written explicitly in the source.

## Edge roles

Every edge has an ordinal and an `AstEdgeRole`. The ordinal preserves child
order. A specific role is used when the relationship has stable meaning;
otherwise the edge remains `Child`.

| Role | Meaning |
| --- | --- |
| `Child` | generic Clang traversal relationship |
| `Declaration` | declaration owned by a declaration statement |
| `Parameter` | function parameter |
| `Body` | function or loop body, or the materialized coroutine body |
| `Initializer` | variable initializer, constructor initializer expression, control-statement initializer, or the common expression of GNU `left ?: right` |
| `Condition` | condition of a branch, loop, switch, or conditional expression |
| `Increment` | classic or range-for increment expression |
| `ThenBranch`, `ElseBranch` | branches of `if` or a conditional expression |
| `LeftOperand`, `RightOperand` | operands of a binary expression |
| `Operand` | operand of a unary, indirect-goto, or coroutine expression |
| `Callee` | called expression |
| `Argument` | call argument |
| `ReturnValue` | value returned by `return` or `co_return` |
| `Range` | range expression of a range-for statement |
| `LoopVariable` | loop-variable declaration of a range-for statement |
| `CaseValue`, `CaseRangeEnd`, `CaseBody` | parts of a case label, including Clang's case-range extension |
| `DefaultBody` | body of a default label |
| `LabelBody` | statement following a source label |
| `LambdaBody` | body of a lambda expression |
| `TryBody`, `Handler`, `CatchBody` | parts of C++ exception handling |
| `ExceptionDeclaration` | declaration introduced by a typed catch handler |
| `ThrownValue` | value of a throw expression |

## Stored metadata

Nodes can carry:

- exact Clang node name;
- unqualified and qualified declaration names, plus directly resolved callees;
- printed type name;
- literal source spelling;
- operator spelling;
- direct-goto target and source-label names;
- address-taken and assembly-goto target labels;
- original case-label spelling;
- caught type and whether a handler is catch-all;
- selected or dependent state for `if constexpr` and
  `__builtin_choose_expr`;
- whether an inline assembly statement is `asm goto`;
- whether a wrapper expression is control-flow transparent;
- whether an operation is conservatively considered potentially throwing;
- whether a direct call is known not to return normally;
- whether a function declaration is an ordinary function, constructor, or
  destructor;
- whether range initialization, comparison, element access, and increment may
  throw;
- whether child expressions occur in an unevaluated context;
- a source range;
- a separate presentation label.

Function metadata contains a Clang USR-based identity, unqualified and
qualified names, a printed function-type signature, and the definition range.
DOT cluster labels combine the qualified name and signature so overloads remain
visually distinguishable.

Source ranges are half-open byte offsets `[begin, end)` into the main source
file. Expansion locations are used for macros. Invalid ranges, ranges crossing
files, and ranges outside the main file are omitted. Offsets refer to bytes,
not Unicode code points or lines and columns.

The current DOT view intentionally exposes concise labels, node categories,
function clusters, and semantic edge labels. It does not serialize every
metadata field.

## What the test suite covers

Unit tests check:

- directed-graph IDs, insertion order, adjacency, roots, and invalid IDs;
- function-AST root and tree invariants;
- DOT identifier validation, escaping, structure, and writer lifecycle;
- deterministic AST presentation and output-stream failure propagation.

Golden tests run the complete native pipeline and compare byte-for-byte DOT for
these examples:

| Example | Main behavior covered |
| --- | --- |
| `basic` | multiple functions, parameters, local declarations, binary expressions, and returns |
| `branches` | nested conditions, calls, short-circuit operands, and conditional expressions |
| `clang_extensions` | indirect goto, case ranges, inline assembly, `asm goto`, GNU conditionals, compile-time choice expressions, and statement expressions |
| `constexpr` | concrete and dependent `if constexpr` branches |
| `control_flow` | `if`, classic `for`, `while`, `do`, `switch`, `case`, `default`, `break`, and `continue` |
| `coroutines` | coroutine body materialization, `co_await`, `co_yield`, and `co_return` |
| `empty` | a valid translation unit with no collected function |
| `expressions` | unary, binary, conditional, cast, pointer, subscript, unevaluated, and assignment expressions |
| `function_definitions` | defaulted and deleted special members, destructor, overloaded call operator, explicit specialization, and exclusion of an implicit template instantiation |
| `lambdas` | captures, throwing capture initializers, lambda body ownership, parameters, and invocation |
| `literals_and_escaping` | string and character literals plus safe DOT escaping |
| `local_classes` | local record structure, constructor initializer execution, and isolation of constructor and method bodies from the enclosing function |
| `loops` | focused loop forms, nesting, break, and continue |
| `main_file_only` | exclusion of a function definition from an included header |
| `range_for` | range-for structure and compiler-created traversal details |
| `switch_and_goto` | switch cases, fallthrough, direct goto, labels, and call conditions |
| `templates_and_methods` | primary function templates, `if constexpr`, fields, and member functions |
| `exceptions_and_labels` | `try`, `catch`, `throw`, labels, and direct `goto` |
| `unreachable` | AST retention for statements following an unconditional transfer |

An additional integration test verifies that invalid C++ produces diagnostics,
a non-zero status, and no partial DOT output.

## Known gaps

The following behavior does not yet have a dedicated regression test:

- macro-expanded function definitions and exact source-offset metadata;
- multi-file `ClangTool` runs and fallback identity generation when Clang
  cannot create a USR;
- simulated allocation failures for the graph's exception-safety rollback.

These are coverage gaps, not promises that the current output is wrong. Unknown
or unclassified nodes are still retained under their generic project category.
New dedicated tests should be added before downstream analysis starts relying
on the detailed shape of one of these constructs.

## Deliberate non-goals

The AST graph does not currently model:

- the complete translation-unit AST outside collected function definitions;
- types, template arguments, attributes, comments, or preprocessor directives
  as independent graph nodes;
- name lookup, overload sets, symbol-reference edges, data flow, or control
  flow;
- a DOT shape that remains byte-identical across different Clang major
  versions.

CFG and CDG construction are separate analysis stages. The CFG implementation
is documented in [CFG graph model and coverage](cfg-model.md). Both stages use
project-owned graph models and algorithms rather than `clang::CFG` or LLVM
analysis passes.
