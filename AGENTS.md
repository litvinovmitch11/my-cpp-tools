# AGENTS.md

## Purpose

`my-cpp-tools` visualizes static analysis of C++ source code. The monorepo has
three components:

- `cpp/`: Clang adaptation, project-owned graph models, analyses, and native
  tools;
- `backend/`: Go HTTP API and bounded native-process orchestration;
- `web/`: React/TypeScript editing and graph visualization.

The current browser product exposes AST end to end. The native C++ layer also
contains the project-owned CFG foundation. CDG is not implemented.

## Required agent context

Before changing the repository, read [`.agents/README.md`](.agents/README.md)
and the task-specific documents it references. They contain the durable
architecture, exact C++ contracts, validation workflow, security boundaries,
current milestone, and staged roadmap.

Human-facing documentation has a different purpose:

- [`README.md`](README.md): setup, commands, API, and product behavior;
- [`docs/architecture.md`](docs/architecture.md): concise system design;
- [`docs/ast-model.md`](docs/ast-model.md): AST semantics and coverage;
- [`docs/cfg-model.md`](docs/cfg-model.md): CFG semantics and coverage.

Do not place agent restoration instructions, temporary plans, or raw
conversation history in human-facing documentation.

## Non-negotiable architecture

1. Clang may parse C++ and provide its AST, source locations, and semantic
   information.
2. CFG must be built manually from the project-owned AST. Do not use
   `clang::CFG`, LLVM analysis passes, compiler CFG output, or a third-party
   CFG builder.
3. CDG must eventually be built manually from the project-owned CFG and
   project-owned control-dependence algorithms.
4. Graphviz is a layout/rendering dependency only.
5. Go orchestrates analysis and owns API contracts; it must not duplicate
   native analysis algorithms.
6. The web client renders and interacts with results; it must not become the
   source of truth for analysis semantics.
7. Compiler dependencies stop at the Clang adapter and CLI composition roots.
   Core graph, source, AST, CFG, CDG, DOT, and presentation modules remain
   independent of Clang and LLVM.
8. Analysis support must not disappear downstream: every valid function AST
   produces a CFG, and once CDG is implemented every valid function CFG must
   produce a CDG. Downstream models may omit details irrelevant to their
   analysis, but must not reject an otherwise valid upstream graph.

The intended analysis dependency direction is:

```text
graph/source -> ast -> cfg -> cdg
```

`dot` is standard-library-only. `presentation` may depend on domain models and
`dot`; domain models must not depend on presentation or tools.

## Scope guard

Unless a task explicitly expands scope:

- keep the browser product on AST;
- keep CFG native-only;
- leave CDG as a neutral placeholder;
- do not add real-time analysis or unrelated graph types;
- do not expose the local-only backend to untrusted networks.

## Repository-wide rules

- Use English for code, comments, documentation, diagnostics, tests, commit
  messages, and GitHub metadata.
- Prefer domain-specific names and observable-behavior test names.
- Treat source code and native-tool output as untrusted input.
- Keep analysis output deterministic and escape all user-derived DOT values.
- Add regression tests for fixes when practical; cover failure paths as well
  as successful behavior.
- Do not commit generated build directories, dependency directories, coverage
  artifacts, reports, logs, local environment files, or secrets.
- Do not commit, push, merge, publish, change GitHub settings, or create a
  release unless the project owner explicitly asks.

## Working method

1. Inspect the relevant implementation, tests, current diff, and the matching
   agent contract document.
2. State assumptions when they change API or analysis semantics.
3. Implement the smallest coherent change while preserving component and
   dependency boundaries.
4. Run focused checks during iteration and the required completion checks from
   [`.agents/workflow.md`](.agents/workflow.md).
5. Review the final diff for generated files, language violations, internal
   path leaks, unrelated edits, and undocumented limitations.

Stable project context belongs in `.agents/`; user and contributor explanations
belong in `README.md` and `docs/`; temporary work belongs in the active Codex
task context.
