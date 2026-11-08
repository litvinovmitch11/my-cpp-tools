# Native C++ contracts

## Analysis and compiler boundary

- Clang may parse C++ and expose AST, source locations, and semantic facts.
- The project builds CFG manually from its owning AST and CDG manually from
  its owning CFG. Do not use `clang::CFG`, LLVM analysis passes,
  compiler-generated CFG/CDG output, or third-party builders.
- `graph`, `source`, `ast`, `cfg`, `cdg`, `dot`, and `presentation` are
  compiler-independent. Their headers, implementations, target interfaces,
  stored values, and tests must not depend on Clang/LLVM headers, libraries,
  macros, RTTI helpers, or compiler objects.
- Clang/LLVM dependencies are confined to `cpp/src/clang`, the narrow adapter
  entry point under `cpp/include/mycpptools/clang`, and CLI composition roots.
  Forward declarations are allowed at that entry point; compiler types must
  not leak into returned models.
- CMake dependencies are `PRIVATE` by default. Use `PUBLIC` only when a
  consumer compiles against a project-owned public type. Compiler include
  directories and libraries must not propagate through core targets.
- `AstNodeMetadata::ConcreteKind` is diagnostic and presentation data only.
  If analysis needs a fact, add a project-owned enum, edge role, or typed
  owning metadata field instead of inspecting a class-name string or Clang
  object.

## Design and ownership

- Prefer composition to graph-domain inheritance. `DirectedGraph` owns generic
  storage; function graph types own validated domain graphs and function
  metadata; builders transform between models.
- Completed graph models own all data and expose no borrowed compiler state.
- Domain layers do not depend on presentation or tools. Presentation can read
  domain models but cannot construct or mutate analysis semantics.
- CLI executables are thin composition roots: parse options, invoke services,
  serialize results, report diagnostics, and choose an exit code.
- Diagnostics go to standard error. DOT goes to standard output only on
  success. Output is deterministic for the same source and reference
  toolchain, and every user-derived value is escaped.

## AST foundation

- Production path: `Clang AST -> FunctionAstGraphs -> GraphDotSerializer ->
  AstPrinter stdout`. Do not add direct Clang-to-DOT traversal.
- A function AST is a non-empty rooted ordered tree. The root is a function
  declaration; every other node has one parent; child ordinals are contiguous;
  all nodes are reachable.
- Collect user-written function definitions whose expansion location is in the
  main file. Include methods, constructors, destructors, operators, primary
  function templates, explicit specializations, and explicitly defaulted or
  deleted functions.
- Exclude implicit functions, implicit template instantiations, duplicate
  function IDs, and lambda call operators as separate function graphs.
- Keep lambdas and their bodies in the enclosing AST. Skip implicit
  declarations as nodes while continuing traversal; preserve statements and
  expressions, including implicit expressions.
- A nested function definition such as a local-class method belongs only to
  its own graph. Do not duplicate its declaration or body in the enclosing
  function graph.
- Source ranges are half-open byte offsets into the main source file using
  expansion locations. Invalid, cross-file, and non-main-file ranges are
  unavailable.
- Keep USR-based function identity separate from qualified name and printed
  signature. Cluster labels include name and signature to distinguish
  overloads.
- Unknown parser nodes stay in the graph with their concrete name as metadata;
  downstream logic branches only on `AstNodeKind` and typed metadata.
- `buildFunctionAstGraphs` is transactional across a complete `ClangTool`
  run: success replaces the output, failure leaves it unchanged.

See [`docs/ast-model.md`](../docs/ast-model.md) for the public semantic and
coverage reference.

## CFG foundation

- Production native path: `FunctionAstGraphs -> FunctionCfgGraphs ->
  GraphDotSerializer -> CfgPrinter stdout`. There is no Clang-to-CFG or
  Clang-to-DOT shortcut.
- Every valid `FunctionAstGraph` produces a CFG. Control-relevant project kinds
  are explicit; other syntax is an atomic operation or transparent wrapper.
- Once CDG is implemented, every valid `FunctionCfgGraph` must produce a CDG.
  CDG may omit nodes irrelevant to control dependence, but it must not reject a
  valid CFG merely because the originating AST construct was represented at a
  higher level.
- Each function CFG has exactly one entry and one exit. Conditions, indirect
  goto, `asm goto`, and potentially throwing operations may branch. Other
  transfers have one resolved successor.
- Retain unreachable nodes and derive reachability from entry.
- Model structured flow, short-circuit expressions, exceptions, coroutines,
  direct and indirect goto, and `asm goto` using only project node kinds,
  roles, and typed metadata.
- Non-dependent `if constexpr` builds only its selected branch; dependent
  primary templates preserve both branches. Lambda bodies are opaque in their
  enclosing function CFG.
- Range-for uses project-owned high-level condition and increment nodes, not
  compiler desugaring declarations, while retaining typed exceptional-flow
  facts for initialization, comparison, element access, and increment.
- Constructor initializer expressions run in semantic AST order before the
  body. Function-try-block handlers cover both initializer and body flow;
  constructor and destructor handlers implicitly rethrow when they complete.
- Direct calls known not to return have no normal successor. Assignment
  evaluation preserves right-before-left sequencing even in a condition.
- Normalize unspecified sibling expression order to stable AST child order.
  Implicit cleanup and compiler lowering remain documented abstractions, not
  rejected constructs.
- Coroutine flow stays high level; compiler-generated protocol calls and their
  exceptional edges are a documented abstraction rather than copied lowering.
- CFG collection is transactional: either all function graphs are returned or
  a typed error is returned and no partial DOT is emitted.
- Leave `CdgPrinter` and the CDG library neutral until the CDG model and
  algorithms are explicitly designed.

See [`docs/cfg-model.md`](../docs/cfg-model.md) for the public semantic and
coverage reference.
