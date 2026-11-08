# Durable architecture context

## Mission

`my-cpp-tools` is both a learning project and an engineered visualization tool
for C++ static analysis. It deliberately implements analysis algorithms in the
repository rather than wrapping compiler-produced CFG or CDG output.

The long-term analysis sequence is:

```text
C++ source
    -> Clang parser and semantic data
    -> project-owned function AST
    -> project-owned CFG
    -> project-owned CDG
    -> DOT presentation
```

The monorepo separates responsibilities:

- native C++ owns parsing adaptation and all static-analysis semantics;
- Go owns the HTTP contract, limits, native-process orchestration, and safe
  error mapping;
- React/TypeScript owns editing, requests, rendering, interaction, and
  accessibility.

Analysis semantics must not migrate into Go or the browser.

## Current milestone

The shipped alpha path is AST end to end:

```text
C++ source -> Go API -> AstPrinter -> DOT -> Graphviz Web Worker -> SVG
```

The active native milestone adds the project-owned CFG foundation:

```text
Clang AST -> FunctionAstGraphs -> FunctionCfgGraphs -> CfgPrinter -> DOT
```

CFG is not yet exposed by Go or web. CDG remains a neutral placeholder. The
next planned product change is a separate MR that exposes CFG through the
backend and web without moving analysis behavior out of C++.

## Repository and dependency map

Compiler-independent native modules:

| Module | Responsibility | Project dependencies |
| --- | --- | --- |
| `graph` | generic directed storage and typed IDs | standard library |
| `source` | function identity and byte ranges | standard library |
| `ast` | owning AST domain and validation | `graph`, `source` |
| `cfg` | owning CFG and AST-to-CFG transformation | `ast`, `graph`, `source` |
| `cdg` | future CFG-to-CDG transformation | `cfg` |
| `dot` | safe DOT syntax writer | standard library |
| `presentation` | visual policy and AST/CFG serialization | domain models, `dot` |

Compiler-aware native modules:

| Module | Responsibility |
| --- | --- |
| `clang_ast` | collect Clang functions and map compiler nodes to owning AST values |
| `AstPrinter` | compose Clang tooling, AST builder, and presentation |
| `CfgPrinter` | compose Clang tooling, AST builder, CFG builder, and presentation |
| `CdgPrinter` | neutral executable placeholder until CDG is designed |

Dependencies point forward through analysis stages. Domain layers never depend
on presentation, tools, backend, or web. Compiler includes and link libraries
stay private to the adapter and composition roots.

## Object model

`DirectedGraph<NodePayload, EdgePayload>` owns generic graph storage. Its
nested ID types prevent mixing identifiers from graph specializations.

`AstGraph`, `CfgGraph`, and the future `CdgGraph` are aliases with different
payloads, not an inheritance hierarchy. Their domains have different
invariants and are transformed into one another rather than substituted
polymorphically.

`FunctionAstGraph` and `FunctionCfgGraph` compose validated graphs with owning
function metadata and expose read-only access after construction. Builders are
transformations, not subclasses. Collection builders are transactional and do
not publish partially built collections.

Presentation is also separate. `GraphDotSerializer` reads validated graphs and
applies visual policy. `DotWriter` owns only DOT syntax and escaping.

## Decisions that future changes must preserve

1. The repository was split into native C++, Go, and web so analysis,
   orchestration, and interaction remain independently testable.
2. The first complete product path exposed AST through a bounded Go runner and
   browser worker.
3. Direct Clang-to-DOT traversal was replaced by an owning project AST to
   separate compiler lifetime from presentation and later analyses.
4. Function identity, source ranges, graph storage, and DOT writing became
   project-owned values and modules.
5. AST collection gained explicit function-selection, ownership, tree,
   source-range, lambda, local-class, and transaction contracts.
6. CFG was implemented as a manual transformation from `FunctionAstGraph`,
   never from Clang CFG or concrete compiler node-name strings.
7. Language examples were unified in `cpp/examples`; each source is exercised
   by AST and CFG pipelines with separate golden output.

## Current deliberate limits

- The browser and HTTP API expose AST only.
- CFG is native-only.
- CDG has no model or algorithm.
- Native parsing is bounded but not sandboxed; the service is local-only.
- Golden graph shape targets the documented Clang major version.
- Graphs and analyses are function-local; interprocedural analysis requires a
  separate design.

## Maintainability watchpoints

The repository is not currently architecturally tangled: dependency direction
is acyclic, compiler integration is isolated, domain models own their data, and
tools remain thin. Two native implementation hotspots must nevertheless be
watched before they grow further:

- `FunctionCfgBuilder` is a cohesive AST-to-CFG transformation, but its large
  implementation distributes expression semantics across ordinary-expression,
  condition, and control-relevance paths. It also carries nested loop, switch,
  exception, and label context as mutable per-build state.
- `ClangAstMapper::makeNode(const clang::Stmt&)` linearly combines several
  independent metadata-enrichment concerns and will become difficult to extend
  if more semantics are added without extracting focused private helpers.

These are recorded risks, not a request to refactor the current CFG milestone.
Do not expand either hotspot blindly. When the next change touches one, first
assess whether duplicated dispatch, hidden context, or cognitive complexity is
increasing; if so, perform the smallest behavior-preserving decomposition before
adding further semantics. Avoid a class-per-node hierarchy or registry-based
dispatch unless a concrete need proves simpler local helpers insufficient.

## Planned stages

1. Review and merge the native CFG foundation.
2. Add a versioned backend CFG contract and bounded native runner path.
3. Add an AST/CFG selector and CFG rendering to web while preserving worker
   isolation, sanitization, cancellation, and accessibility.
4. Design CDG semantics, post-dominators, and control dependence from the
   project-owned CFG before implementation.
5. Consider public deployment only after explicit process, filesystem, memory,
   CPU, and concurrency isolation is designed.
