# Analysis support and guarantees

This page separates behavior the project guarantees from deliberate
abstractions and capabilities that do not exist yet. A valid construct is not
“unsupported” merely because the graph represents it at a higher level than a
compiler would.

The complete construct-by-construct AST, CFG, and future CDG coverage matrix is
in the [README](../README.md#c-ast-construct-coverage). Its **Shadowed** entries
are the construct-level form of the documented high-level abstractions below.
Invalid source code and malformed internal graphs are tested failure cases, not
language-support categories.

Support must not disappear between stages. Every valid function AST produces a
CFG. Once CDG exists, every valid function CFG must produce a CDG. A downstream
stage may deliberately omit details that are irrelevant to its analysis, but it
must not reject a valid upstream graph merely because its representation is
more abstract.

## Supported AST foundation

The following behavior is part of the tested AST contract:

- collection of user-written definitions in the main file, including methods,
  constructors, destructors, operators, primary templates, explicit
  specializations, and explicitly defaulted or deleted functions;
- exclusion of header-only definitions, implicit functions, implicit template
  instantiations, duplicate identities, and separate lambda call-operator
  graphs;
- owning rooted trees with one parent per non-root node, contiguous child
  order, source byte ranges, stable function identities, and no borrowed Clang
  state;
- lambda bodies in their enclosing function AST and local-class methods only
  in their own function graph;
- project-owned node kinds, edge roles, and typed metadata for every fact used
  by CFG.

The exact node and edge vocabulary is in [AST graph model and
coverage](ast-model.md).

## Supported CFG semantics

| Area | Guaranteed behavior | Regression corpus |
| --- | --- | --- |
| Function boundaries | Exactly one entry and exit; unreachable nodes retained and marked by reachability | `basic`, `unreachable` |
| Branches | `if`, nested conditions, `&&`, `||`, `!`, and conditional expressions preserve branching | `branches`, `expressions` |
| Assignment | Control-relevant RHS is evaluated before LHS, including assignment conditions | `expressions` |
| Loops | Classic `for`, range-for, `while`, `do`/`while`, nesting, `break`, and `continue` | `loops`, `control_flow`, `range_for` |
| Range protocol | High-level initialization, comparison, element access, and increment retain exceptional flow | `range_for` |
| Switch | Cases, default, ranges, and fallthrough have explicit branch edges | `control_flow`, `switch_and_goto`, `clang_extensions` |
| Labels and goto | Direct and indirect goto, address-taken labels, unresolved-label errors, and unreachable regions | `switch_and_goto`, `clang_extensions`, `exceptions_and_labels` |
| Exceptions | `try`, typed and catch-all handlers, throw, rethrow, and nearest-handler routing | `exceptions_and_labels`, `range_for` |
| Function try blocks | Constructor initialization is protected; constructor/destructor handlers implicitly rethrow on completion | `local_classes` |
| Calls | Potentially throwing calls have exceptional flow; direct `[[noreturn]]` calls have no normal successor | `branches`, `unreachable` |
| Compile-time selection | Non-dependent `if constexpr` and builtin choice keep only the selected branch; dependent forms preserve both | `constexpr`, `templates_and_methods`, `clang_extensions` |
| Lambdas | Capture initializers execute in the enclosing CFG; the lambda body remains opaque there | `lambdas` |
| Coroutines | `co_await`, `co_yield`, and `co_return` use high-level suspend, resume, and transfer flow | `coroutines` |
| Assembly | Ordinary assembly is atomic; `asm goto` has label branches and fallthrough | `clang_extensions` |
| Unknown syntax | Valid AST nodes without specialized control behavior remain atomic or preserve child flow | all examples |

Focused unit tests assert semantic ordering, edge kinds, reachability,
transactionality, exceptional targets, and graph invariants. Golden tests then
exercise every example through Clang, both graph builders, and deterministic
DOT serialization.

## Supported with deliberate abstraction

These behaviors work according to a narrower documented guarantee:

| Area | What is guaranteed | What is deliberately omitted |
| --- | --- | --- |
| Calls | Intraprocedural call operations and conservative exceptional edges | Callee CFG expansion and interprocedural propagation |
| Exceptions | Possible handler routing and uncaught flow | Complete C++ exception type matching and `noexcept` proof |
| Range-for | Stable high-level protocol phases | Clang's hidden declarations and exact lowering |
| Coroutines | Ready, suspend, resume, and return structure | Generated state machine and hidden protocol-call exceptions |
| Object lifetime | User-visible initialization and operations | Implicit destruction, temporary cleanup regions, and lifetime analysis |
| Evaluation order | Standard-mandated sequencing; otherwise stable AST child order | Enumeration of every order permitted by C++ |
| Unknown parser nodes | Retention as atomic operations or transparent child flow | Specialized semantics until a project-owned kind or metadata field is added |

These are valid CFGs within the project model. They must not be reported as
parse failures or unsupported C++.

## Not implemented

The project currently makes no claim to provide:

- CDG construction, post-dominators, or control-dependence results;
- data-flow, def-use, alias, value-range, or lifetime analysis;
- interprocedural CFG composition or call-graph analysis;
- complete type, template-argument, preprocessor, comment, or attribute graphs;
- CFG through the Go API or browser interface;
- public multi-tenant execution isolation.

These require explicit designs and tests before they can move into Supported or
Abstracted status.

## Running each evidence layer

```bash
ctest --test-dir cpp/build -L contract --output-on-failure
ctest --test-dir cpp/build -L golden --output-on-failure
ctest --test-dir cpp/build -L negative --output-on-failure
ctest --test-dir cpp/build -L architecture --output-on-failure
```

The complete CTest run, format check, clang-tidy, coverage, and release build
remain the merge gate. See the [README](../README.md) for all commands.
