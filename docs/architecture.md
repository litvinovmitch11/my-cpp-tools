# Architecture

`my-cpp-tools` turns C++ source code into visual program-analysis graphs. The
project is deliberately split into parsing and analysis, service orchestration,
and browser presentation so each concern can evolve and be tested separately.

## System overview

The browser exposes both analysis pipelines through one selector and rendering
path:

```text
C++ source + AST/CFG/CDG selection
    -> versioned Go API route
    -> AstPrinter, CfgPrinter, or CdgPrinter
    -> AST, CFG, or CDG DOT
    -> Graphviz Web Worker
    -> sanitized interactive SVG
```

The backend also exposes the native CDG pipeline:

```text
POST /api/v1/cdg
    -> shared bounded native runner
    -> Clang AST
    -> project-owned function AST
    -> project-owned CFG
    -> project-owned CDG
    -> DOT
```

## Component responsibilities

### Native C++

The C++ component adapts Clang's parser output into owning project models and
contains all static-analysis semantics. `AstPrinter` writes an AST graph;
`CfgPrinter` transforms the same AST model into a CFG, and `CdgPrinter`
derives and writes its control-dependence graph.

Clang supplies syntax, source locations, and semantic facts. It does not supply
the project's CFG. The implementation does not use `clang::CFG`, LLVM analysis
passes, or third-party CFG/CDG builders.

### Go backend

The backend exposes separate versioned AST, CFG, and CDG routes, validates requests,
manages one shared bounded pool of native processes, and maps failures to safe
errors. It does not recreate native analysis algorithms and never compiles or
executes submitted programs. Native exit codes distinguish rejected source
from internal analysis failures.

### Web client

The browser provides the editor, AST/CFG/CDG selection, and graph interaction.
Graphviz runs in a Web Worker, and generated SVG is sanitized before display.
The client owns presentation and interaction, not analysis meaning.

## Native module layout

| Module | Purpose |
| --- | --- |
| `graph` | Generic directed storage and strongly typed node and edge IDs |
| `source` | Function identity and source byte ranges |
| `ast` | Owning function AST and its invariants |
| `cfg` | Owning CFG and the AST-to-CFG transformation |
| `cdg` | Owning CDG, strong post-dominators, and CFG-to-CDG transformation |
| `dot` | DOT syntax and escaping |
| `presentation` | Graph colors, shapes, labels, and serialization |
| `clang_ast` | The only adapter from compiler objects to project AST values |

The analysis direction is one way:

```text
graph/source -> ast -> cfg -> cdg
```

The graph models do not depend on presentation, command-line tools, Go, or the
browser. Clang and LLVM types stay inside the adapter and native tool entry
points; completed AST, CFG, and CDG objects own ordinary project values.

## Graph and ownership design

`DirectedGraph` is reusable storage. AST, CFG, and CDG use it with different node and
edge payloads, but they are not subclasses of one another: they describe
different domains and have different invariants.

`FunctionAstGraph`, `FunctionCfgGraph`, and `FunctionCdgGraph` combine function
information with a validated graph. Builders transform one owning model into
another. A failed collection does not publish partial results.

DOT generation is a separate presentation step. `GraphDotSerializer` chooses
the visible graph policy, while the low-level writer is responsible only for
valid syntax and escaping. This keeps visualization choices out of analysis
algorithms.

## Supported analysis

The function AST includes user-written definitions from the main source file,
including functions, methods, constructors, destructors, operators, templates,
and explicit specializations. It keeps statements and expressions, owns source
ranges and semantic metadata, and isolates nested function bodies. The exact
selection rules, node kinds, edge roles, examples, and known gaps are described
in [AST graph model and coverage](ast-model.md).

The CFG handles ordinary sequencing, branches, all loop forms, switch and
fallthrough, short-circuit expressions, direct and indirect goto, exceptions,
constructor initializers, coroutines, inline assembly, unreachable code, and
compile-time branch selection. Syntax without specialized control behavior is
kept as an atomic operation rather than rejected. See
[CFG graph model and coverage](cfg-model.md) for the complete contract and its
deliberate abstractions.

The native CDG retains one node per CFG node and derives strong,
termination-sensitive control dependence. Infinite paths, no-return terminals,
unreachable components, exceptional flow, indirect goto, and `asm goto` are
part of the model. See [CDG model and semantics](cdg-model.md).

## Deliberate limits

- Analysis is currently function-local and intraprocedural.
- The browser and API expose AST, CFG, and CDG.
- The CFG is source-oriented: it does not expand implicit destruction,
  temporary cleanup, complete exception type matching, or compiler coroutine
  lowering.
- Exact golden graph shape targets the documented Clang major version.
- Native parsing has time and output bounds but is not an OS sandbox, so the
  service remains local-only.

## Next stages

1. Refine CDG-specific presentation and interaction without moving analysis
   semantics into the browser.
2. Consider public deployment only after explicit process, filesystem, CPU,
   memory, and concurrency isolation.

Build, test, and local-run instructions are in the [README](../README.md).
