# CFG graph model and coverage

This document describes the project-owned control-flow graph produced by
`CfgPrinter`. The browser and Go API still expose only AST graphs; the CFG is
the native foundation for the next product stage.

The explicit Supported, Abstracted, and Not implemented matrix is documented
in [Analysis support and guarantees](analysis-support.md).

## Pipeline and ownership

```text
Clang AST
    -> FunctionAstGraph
    -> project-owned CFG builder
    -> FunctionCfgGraph
    -> GraphDotSerializer
    -> DOT
```

Clang is used only to parse source and populate the project-owned AST. The CFG
builder consumes `FunctionAstGraph`; it does not access Clang objects, inspect
Clang node-name strings, or use `clang::CFG`, LLVM analysis passes, compiler CFG
dumps, or third-party CFG builders.

This is an architectural invariant: every valid function AST accepted by the
AST layer must produce a CFG. A known `AstNodeKind` is modeled explicitly when
it changes control flow. Other declarations, statements, and expressions are
kept as atomic operations, and unknown statement wrappers preserve the control
flow of their children. New Clang syntax therefore cannot be rejected merely
because the CFG has no dedicated spelling for it.

Completed CFGs own their data and retain only project node IDs, strings, and
byte ranges. Building a collection is transactional: either every function CFG
is returned, or one typed structural error is returned and no partial DOT is
written.

## Graph contract

Each function has exactly one `Entry` and one `Exit`. `Entry` is the graph root,
has no predecessors, and has one flow successor. `Exit` has no successors.

CFG nodes have these project-owned kinds:

| Kind | Meaning |
| --- | --- |
| `Entry`, `Exit` | unique function boundaries |
| `Operation` | an atomic declaration, expression, call, or ordinary statement |
| `Condition` | a Boolean, switch, exception, or assembly decision |
| `Transfer` | an explicit non-local transfer inside the function |
| `Anchor` | a structural target for an empty region, label, join, or suspension point |

Transfer metadata distinguishes `return`, `break`, `continue`, direct and
indirect `goto`, `throw`, and `co_return`.

Edges distinguish normal flow, true and false branches, switch cases and
default, exceptions, indirect and assembly branches, assembly fallthrough, and
coroutine suspend and resume flow. Switch and label-target edges retain their
source spelling for DOT labels. A potentially throwing operation may have one
normal successor and one exceptional successor. An indirect-goto transfer may
have several target successors; other transfer nodes have one successor.

Nodes may retain their originating AST node ID and source range so later
analyses can relate CFG results to source without borrowing compiler state.
The graph also retains unreachable nodes. `computeReachableNodes` derives
reachability from `Entry`; DOT renders unreachable nodes and their outgoing
edges with a dashed muted style.

## Construction rules

- Function parameters are descriptive AST data, not executable CFG operations.
- Constructor initializer expressions execute before the constructor body;
  function-try-block handlers cover exceptions from both regions. Reaching the
  end of a constructor or destructor function-try-block handler implicitly
  rethrows the active exception.
- A variable initializer executes before its declaration operation.
- `return` and `co_return` evaluate their value before transferring to `Exit`.
- A direct call to a function declared `[[noreturn]]` has no normal successor.
- `break`, `continue`, direct `goto`, source labels, switch fallthrough, and
  nested loop/switch targets are resolved explicitly.
- Indirect goto branches to address-taken labels in the function. If the AST
  contains no address-taken label, the builder conservatively uses all labels;
  a function without any label uses an `unknown` branch to `Exit`.
- `if`, classic `for`, `while`, `do`/`while`, range-for, and `switch` are built
  directly from semantic AST edge roles.
- A range-for uses high-level “range has next” and “advance range” operations;
  it does not depend on Clang's hidden desugaring declarations. High-level
  range initialization, comparison, element access, and increment retain
  conservative exceptional flow.
- Built-in `&&`, `||`, and `?:` preserve short-circuit control flow. The comma
  operator preserves left-to-right sequencing.
- When C++ leaves the order of sibling expression evaluation unspecified, the
  CFG uses stable AST child order as a deterministic normalization. All sibling
  expressions remain present; the order is not a claim about one concrete
  runtime execution.
- Assignment expressions evaluate control-relevant right-hand-side operations
  before control-relevant left-hand-side operations, including when the
  assignment itself is used as a condition.
- Unevaluated operands such as those below `sizeof`, `noexcept`, and `requires`
  remain AST data but do not become executable CFG operations. A non-dependent
  `__builtin_choose_expr` builds only its selected expression.
- A lambda expression is atomic in its enclosing function. Its body does not
  leak into that function's CFG, while control flow in capture initializers is
  evaluated before lambda construction.
- For a non-dependent `if constexpr`, only the selected branch is built. A
  dependent primary template keeps both branches.
- `try`, typed handlers, catch-all handlers, explicit `throw`, and rethrow are
  represented with exceptional edges. Calls, construction, allocation,
  deletion, `dynamic_cast`, and `typeid` are conservatively marked as
  potentially throwing by the AST adapter and target the nearest handler
  dispatch or function exit.
- `co_await` and `co_yield` use high-level ready, suspend, and resume flow;
  `co_return` transfers to the function exit. Compiler-generated coroutine
  state-machine details and exceptional flow from hidden protocol calls are
  intentionally not copied.
- Ordinary inline assembly is atomic. `asm goto` has explicit label branches
  and a fallthrough path.

No mandatory merge node is inserted after every branch. Successor fragments
carry their open exits until a real following operation is known. Anchors are
used only when a concrete target is required.

## Abstraction boundaries

The CFG accepts every valid project AST, but acceptance is not the same as
reproducing every compiler lowering detail. The model deliberately remains
source-oriented:

- calls are intraprocedural atomic operations;
- potentially throwing operations are conservative rather than a complete
  language-level `noexcept` proof;
- catch selection is represented as possible handler branches, not a full C++
  type-matching engine;
- object lifetimes, implicit destructor calls, temporary cleanup regions, and
  compiler-generated coroutine machinery are not expanded;
- unspecified sibling evaluation order is normalized to stable AST order;
- data flow and value propagation are separate future analyses.

These are documented abstractions inside a valid CFG, not unsupported AST
constructs or reasons for the builder to fail. Builder errors are reserved for
broken AST invariants, missing required semantic roles or enclosing targets,
unresolved direct labels, and internal CFG invariant violations.

## Test coverage

Every source in `cpp/examples` is passed through both the AST and CFG pipelines.
Golden tests compare deterministic DOT byte for byte. Together the corpus
covers linear code, declarations, expression forms, branches, short-circuiting,
all loop forms, switch fallthrough, direct and indirect goto, labels, local
classes, lambdas, templates, `if constexpr`, exceptions, coroutines, ordinary
assembly, `asm goto`, unreachable regions, literals, escaping, and translation
units with no collected functions.

Unit tests validate graph invariants, reachability, short-circuit construction,
label resolution, exceptional flow, transactional failure, deterministic DOT
presentation, and output failure handling. An integration failure test verifies
invalid C++, diagnostics, non-zero status, and absence of partial DOT output.
