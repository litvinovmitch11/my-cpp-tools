# AGENTS.md

## Project mission

`my-cpp-tools` is a learning and engineering project for visualizing static
analysis of C++ source code. A user enters C++ code in the browser, the Go API
invokes a native Clang-based tool, and the browser renders the returned graph.

The current alpha pipeline is:

```text
C++ source -> Go API -> AstPrinter -> DOT -> Graphviz Web Worker -> SVG
```

The repository is a monorepo with three main components:

- `cpp/`: Clang-based source analysis and graph generation tools.
- `backend/`: Go HTTP API and native-process orchestration.
- `web/`: React/TypeScript editor and graph visualization UI.

Use `README.md` for user-facing setup instructions. Keep this file focused on
stable engineering rules for agents and contributors.

## Original problem and architectural constraints

The long-term goal is to visualize AST, CFG, and CDG graphs for C++ code while
learning and implementing the analysis algorithms in this repository.

The following constraints are non-negotiable unless the project owner changes
them explicitly:

1. Clang may parse C++ and expose its AST, source locations, and semantic
   information.
2. The project must build its CFG manually from AST nodes and project-owned
   traversal/analysis code.
3. The project must build its CDG manually from the project-owned CFG and
   project-owned control-dependence algorithms.
4. Do not use `clang::CFG`, LLVM analysis passes, compiler-generated CFG/CDG
   output, or third-party CFG/CDG builders as the implementation.
5. Graphviz is only a graph layout/rendering dependency. It must not implement
   the analysis itself.
6. Go orchestrates analysis and exposes API contracts; it must not duplicate
   C++ AST, CFG, or CDG algorithms.
7. The web client renders and interacts with graph data; it must not become the
   source of truth for static-analysis semantics.

The current product scope is AST end-to-end. Do not implement CFG, CDG,
real-time analysis, or unrelated graph types unless the task explicitly asks
for them.

## Component boundaries

### C++

- `AstPrinter` accepts a source file and writes DOT to standard output.
- Diagnostics go to standard error and a non-zero exit code signals failure.
- Output must be deterministic for the same source and toolchain.
- Escape every user-derived value before writing DOT.
- Keep analysis logic independent from presentation attributes where practical.
- `CfgBuilder` and `CdgBuilder` are future project-owned implementations, not
  wrappers around compiler-provided graph builders.

### Go backend

- `POST /api/v1/ast` accepts source code and returns DOT or a structured error.
- Treat all source code and native-tool output as untrusted input.
- Never compile or execute submitted user programs.
- Preserve request-size, timeout, diagnostics, and output-size limits.
- Use typed domain errors and avoid leaking internal paths or internal errors.
- The default server address must remain loopback-only until a sandboxed public
  deployment is designed.

### Web

- Send source code through the versioned backend API.
- Run Graphviz in a Web Worker so layout cannot block the UI thread.
- Sanitize generated SVG before inserting it into the DOM.
- Keep request cancellation and stale worker-response protection intact.
- Preserve keyboard-first editor behavior and accessible labels for controls.

## Language and naming

- Use English for code, comments, documentation, UI text, diagnostics, test
  names, commit messages, and GitHub metadata.
- Prefer domain names over generic names such as `data`, `item`, or `manager`.
- Use `AST`, `CFG`, and `CDG` consistently in documentation. Follow each
  language's established casing conventions in identifiers.
- Name tests after observable behavior rather than implementation details.
- Do not add TODO comments without an owner-facing explanation or tracked issue.

## Build and validation commands

Run only the commands relevant to the changed component during iteration. Run
the full end-to-end path before completing a cross-component change.

### C++

```bash
cmake -S cpp -B cpp/build
cmake --build cpp/build --target AstPrinter
cmake --build cpp/build --target format-check
cmake --build cpp/build --target tidy
```

### Go

```bash
cd backend
go test ./...
go test -race ./...
go vet ./...
```

Integration tests that invoke the native tool require `AST_PRINTER_PATH`.

### Web

```bash
cd web
npm ci
npm run check
npm run e2e
```

### Full pipeline

```bash
./scripts/e2e.sh
```

Do not commit generated build directories, dependency directories, coverage
artifacts, Playwright reports, logs, or local environment files.

## Testing expectations

- Add unit tests for new domain logic and error mappings.
- Add integration tests at native-process and HTTP boundaries.
- Add or update E2E tests for user-visible cross-component behavior.
- Every bug fix should include a regression test when practical.
- Test failure paths and limits, not only happy paths.
- Coverage thresholds are quality gates, not a reason to test trivial lines or
  implementation details.

## Security boundaries

- Never commit secrets, tokens, private keys, credentials, or real `.env`
  files.
- Do not print environment variables or secret-bearing configuration in logs.
- Keep GitHub Actions permissions minimal and pin third-party actions.
- Treat Clang as a native parser operating on hostile input. Timeout and output
  limits do not provide filesystem or memory isolation.
- The alpha backend is local-only and must not be exposed to untrusted networks.
- A public deployment requires process isolation, filesystem isolation, memory
  and CPU limits, and bounded concurrency.

## Change workflow

1. Inspect the relevant component and its tests; do not read unrelated files by
   default.
2. State assumptions when a decision changes API or analysis semantics.
3. Implement the smallest coherent change that satisfies the task.
4. Run focused checks while iterating, then the required completion checks.
5. Review the final diff for accidental generated files, language violations,
   internal-path leaks, and unrelated edits.
6. Report remaining limitations explicitly instead of hiding them behind a
   successful test run.

Do not commit, push, merge, publish, change GitHub settings, or create releases
unless the user explicitly requests that action.

## Plans and durable memory

- Keep stable project constraints in `AGENTS.md`.
- Keep user-facing setup and behavior in `README.md`.
- Record durable architectural decisions in focused documents under `docs/`.
- For a cross-component or multi-session MR, create
  `docs/plans/<short-name>.md` with the objective, non-goals, workstreams,
  acceptance criteria, current status, and unresolved decisions.
- Update the plan after meaningful milestones so another task can continue from
  the repository state without the original chat transcript.
- Do not store raw conversation logs or temporary reasoning in the repository.

## Using multiple agents

The primary agent owns the plan, architectural consistency, shared-file edits,
integration, and final verification. Delegate only concrete workstreams that
can proceed independently, for example:

- C++ tests and analysis internals;
- Go API and runner tests;
- web unit tests and accessibility;
- CI/security configuration;
- independent review of a finished diff.

Give each subagent a bounded task, acceptance criteria, relevant paths, and the
current architectural constraints. Avoid assigning multiple agents to the same
files. Subagents should return findings or completed scoped changes to the
primary agent, which reconciles conflicts and runs integration checks.

Use the repository, this file, the current diff, and a concise task brief as
shared memory. Do not copy an entire long conversation into every subagent
unless its history is essential to the task.

## Definition of done

A change is complete when:

- its behavior and boundaries match the requested scope;
- relevant formatters, linters, builds, and tests pass;
- cross-component changes pass the full E2E pipeline;
- tests cover important success and failure behavior;
- public contracts and documentation are updated;
- no secrets, generated artifacts, Russian text, or unrelated edits were added;
- security and operational limitations are stated clearly.
