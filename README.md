# my-cpp-tools

[![CI](https://github.com/litvinovmitch11/my-cpp-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/litvinovmitch11/my-cpp-tools/actions/workflows/ci.yml)
[![Security](https://github.com/litvinovmitch11/my-cpp-tools/actions/workflows/security.yml/badge.svg)](https://github.com/litvinovmitch11/my-cpp-tools/actions/workflows/security.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

`my-cpp-tools` turns C++ source code into interactive program-analysis graphs.
The alpha release provides a complete AST pipeline: edit C++ in the browser,
send it to a local Go API, analyze it with Clang, and inspect the result as an
interactive graph.

> [!WARNING]
> The alpha backend is local-only and must not be exposed to untrusted
> networks. Clang runs as a native process and is not isolated by a container
> or OS sandbox.

![AST viewer with a C++ editor and interactive graph](docs/assets/ast-viewer.png)

## Architecture

```text
React editor
    │ POST /api/v1/ast
    ▼
Go HTTP API ── bounded native-process runner
    │ temporary input.cpp
    ▼
Clang AstPrinter
    │ DOT
    ▼
Graphviz Web Worker ── sanitized SVG ── interactive graph view
```

The monorepo has three components:

- `cpp/` contains the Clang-based AST adapter and the project-owned AST and CFG
  graph tools.
- `backend/` contains the Go API, validation, process limits, and error mapping.
- `web/` contains the React/TypeScript editor and Graphviz visualization.

CFG and CDG remain outside the browser alpha scope. A native `CfgPrinter` now
builds CFGs manually from the project-owned AST. CDG will be built manually from
that CFG. The project does not use `clang::CFG`, LLVM analysis passes, or
third-party CFG/CDG builders.

See [AST graph model and coverage](docs/ast-model.md) for the exact function
selection rules, node and edge categories, tested constructs, and known gaps.
See [CFG graph model and coverage](docs/cfg-model.md) for CFG invariants,
construction rules, semantic abstractions, and tested constructs.
See the [architecture overview](docs/architecture.md) for component ownership,
dependency boundaries, supported analysis, deliberate limits, and the roadmap.
See [analysis support and guarantees](docs/analysis-support.md) for the detailed
per-layer guarantees and their test evidence.

## Prerequisites

- CMake 3.28 or newer;
- LLVM and Clang 22 with development packages (the reference toolchain);
- Go 1.27.3 or newer;
- Node.js 24 or newer;
- `make` and `rg` (ripgrep).

The code may build with another matching LLVM/Clang release, but exact AST shape
is compiler-version-dependent and the checked-in golden output targets Clang 22.
On Ubuntu, the required packages are commonly named `clang-22`,
`clang-format-22`, `clang-tidy-22`, `llvm-22-dev`, and `libclang-22-dev`.

## Start from a fresh clone

```bash
git clone https://github.com/litvinovmitch11/my-cpp-tools.git
cd my-cpp-tools
make bootstrap
make dev
```

Open `http://127.0.0.1:5173`. `make dev` builds `AstPrinter`, starts the Go API
on `127.0.0.1:8080`, starts Vite, and stops both processes together. Use
<kbd>Ctrl</kbd>/<kbd>⌘</kbd> + <kbd>Enter</kbd> to build a graph.

Configuration is environment-based. See [`.env.example`](.env.example) for all
supported local settings; it contains no secrets.

## Build and test

The following commands configure and build all C++ libraries, tools, and tests:

```bash
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Debug
cmake --build cpp/build --parallel
```

If CMake cannot discover LLVM and Clang, provide their package directories:

```bash
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-22 \
  -DLLVM_DIR=/usr/lib/llvm-22/lib/cmake/llvm \
  -DClang_DIR=/usr/lib/llvm-22/lib/cmake/clang
```

Run the native test suite and quality checks with:

```bash
ctest --test-dir cpp/build --output-on-failure
cmake --build cpp/build --target format-check
cmake --build cpp/build --target tidy
./scripts/cpp-coverage.sh
```

The test suite is labeled by evidence layer. For a focused run, use `-L
contract`, `-L golden`, `-L negative`, or `-L architecture` with CTest. The
unfiltered run remains the merge gate.

To build only deployable native artifacts without tests, configure a separate
release directory:

```bash
cmake -S cpp -B cpp/build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build cpp/build-release --parallel
```

Run `AstPrinter` directly with a source file followed by Clang arguments:

```bash
./cpp/build/src/tools/AstPrinter/AstPrinter \
  cpp/examples/control_flow.cpp -- -std=c++20
```

Run the native CFG stage in the same way:

```bash
./cpp/build/src/tools/CfgPrinter/CfgPrinter \
  cpp/examples/loops.cpp -- -std=c++20
```

Golden tests never modify their expected output. After an intentional graph or
presentation change, inspect the failing diff and explicitly update the
affected goldens with:

```bash
cmake --build cpp/build --target update-ast-goldens
git diff -- cpp/tests/golden/ast

cmake --build cpp/build --target update-cfg-goldens
git diff -- cpp/tests/golden/cfg
```

For the complete monorepo, `make bootstrap` installs/configures dependencies,
`make check` reproduces the non-E2E CI gates, and `make e2e` exercises the full
browser-to-native pipeline.

## API

`POST /api/v1/ast` accepts one JSON object:

```json
{
  "code": "int main() { return 42; }"
}
```

A successful response contains DOT:

```json
{
  "dot": "digraph AST { ... }"
}
```

Errors use a stable envelope and may include sanitized compiler diagnostics:

```json
{
  "error": {
    "code": "invalid_source",
    "message": "source code could not be parsed",
    "diagnostics": "input.cpp:1:1: error: ..."
  }
}
```

The API limits source size, graph size, diagnostic size, execution time, and
concurrent native processes. `GET /healthz` is the local health endpoint.

## Developer commands

| Command          | Purpose                                                                  |
| ---------------- | ------------------------------------------------------------------------ |
| `make bootstrap` | Configure C++, download Go modules, install web and browser dependencies |
| `make format`    | Apply Go, C++, and web formatters                                        |
| `make lint`      | Run Go vet, clang-tidy, ESLint, and TypeScript checks                    |
| `make test`      | Run Go race tests, CTest, and web unit tests                             |
| `make coverage`  | Enforce Go, C++, and web coverage thresholds                             |
| `make e2e`       | Exercise browser → API → AstPrinter → graph rendering                    |
| `make check`     | Reproduce the non-E2E CI quality gates locally                           |
| `make dev`       | Start the local development stack with process cleanup                   |

Coverage gates start at 70% for Go, 60% lines for project-owned C++, and 70%
lines/functions plus 60% branches for selected web production code.

## Continuous integration

Pull requests and pushes to `main` run the same repository gates in GitHub
Actions:

- repository hygiene and generated-file checks;
- Go formatting, vet, race tests, and coverage;
- C++ formatting, clang-tidy, build, CTest, and LLVM source coverage;
- web formatting, lint, type checking, unit coverage, and production build;
- a full Playwright end-to-end test after component jobs pass;
- CodeQL, secret scanning, Go vulnerability checks, and npm audit.

The C++ CI job uses Clang 22, so its golden tests match the documented reference
toolchain. Workflow actions are pinned to immutable commit hashes and run with
read-only repository permissions unless a security upload requires more.

## Security model

Submitted code is parsed, never compiled into an executable or run. The backend
uses a private temporary directory, a minimal child environment, bounded output,
a timeout, and bounded concurrency. These controls reduce accidental resource
use but are not a security sandbox. Do not run the service outside a trusted
local development environment.

## Roadmap

- `v0.1` alpha: reliable AST pipeline, repository automation, and quality gates;
- expose the project-owned CFG through the Go API and web client;
- manual CDG construction from the project-owned CFG;
- additional graph interactions and incremental analysis;
- sandboxed packaging and deployment only after explicit isolation work.

Contributor automation should read the project constraints in
[AGENTS.md](AGENTS.md) before changing analysis logic.

## C++ AST construct coverage

This matrix covers every Clang declaration, statement, and expression that can
be encountered while traversing a collected function. **Supported** means the
project has a dedicated representation or lowering. **Shadowed** means the
construct is preserved through a generic project node, an atomic CFG operation,
or another documented high-level abstraction. **Not supported** means the
layer does not currently represent that information. The exact Clang class name
is retained as diagnostic metadata even when its project representation is
shadowed.

| C++ / Clang AST construct | Project AST | CFG | CDG |
| --- | --- | --- | --- |
| Function definitions: free functions, methods and operators | **Supported** — `FunctionDeclaration` | **Supported** — function entry, body and exit | **Not supported yet** |
| Constructors and destructors | **Supported** — `FunctionDeclaration` with function category | **Supported** — initializers, body and function-try-block rules | **Not supported yet** |
| Primary function templates and explicit specializations | **Supported** — collected `FunctionDeclaration` | **Supported** — concrete or dependent control flow | **Not supported yet** |
| Explicitly defaulted or deleted functions | **Supported** — collected `FunctionDeclaration` | **Supported** — valid empty function CFG | **Not supported yet** |
| Function parameters (`ParmVarDecl`) | **Supported** — `ParameterDeclaration` | **Shadowed** — descriptive, not executable | **Not supported yet** |
| Local variables (`VarDecl`) | **Supported** — `VariableDeclaration` | **Supported** — initializer before declaration operation | **Not supported yet** |
| Fields (`FieldDecl`) | **Supported** — `FieldDeclaration` | **Shadowed** — descriptive unless used by an expression | **Not supported yet** |
| Records and local classes (`RecordDecl`) | **Supported** — `RecordDeclaration` | **Shadowed** — declaration is atomic; methods have separate CFGs | **Not supported yet** |
| Other traversed declarations: bindings, enums, aliases, using declarations, access declarations and static assertions | **Shadowed** — `OtherDeclaration` with concrete Clang kind | **Shadowed** — atomic or descriptive operation | **Not supported yet** |
| Compound statements and declaration statements | **Supported** — `CompoundStatement`, `DeclarationStatement` | **Supported** — ordered child sequence | **Not supported yet** |
| Runtime `if` | **Supported** — `IfStatement` with semantic edge roles | **Supported** — true and false branches | **Not supported yet** |
| `if constexpr` | **Supported** — `ConstexprIfStatement` with selection metadata | **Supported** — selected branch or both dependent branches | **Not supported yet** |
| `switch` | **Supported** — `SwitchStatement` | **Supported** — case, default and fallthrough flow | **Not supported yet** |
| `case`, GNU case range and `default` | **Supported** — `CaseStatement`, `DefaultStatement` | **Supported** — labeled branch targets | **Not supported yet** |
| Classic `for` | **Supported** — `ForStatement` | **Supported** — initializer, condition, increment, body, break and continue | **Not supported yet** |
| Range-based `for` | **Supported** — `RangeForStatement` | **Shadowed** — supported high-level range protocol rather than Clang desugaring | **Not supported yet** |
| `while` and `do` / `while` | **Supported** — `WhileStatement`, `DoStatement` | **Supported** — loop condition, back edge, break and continue | **Not supported yet** |
| `return` | **Supported** — `ReturnStatement` | **Supported** — value evaluation followed by transfer to exit | **Not supported yet** |
| `break` and `continue` | **Supported** — `BreakStatement`, `ContinueStatement` | **Supported** — resolved loop or switch transfer | **Not supported yet** |
| Direct `goto` and source labels | **Supported** — `GotoStatement`, `LabelStatement` | **Supported** — resolved target and unreachable regions | **Not supported yet** |
| GNU indirect goto and address-of-label | **Supported** — `IndirectGotoStatement`, `AddressOfLabelExpression` | **Supported** — explicit possible-target branches | **Not supported yet** |
| C++ `try`, typed `catch` and catch-all | **Supported** — `TryStatement`, `CatchStatement` | **Supported** — handler dispatch and function-try semantics | **Not supported yet** |
| `throw` and rethrow | **Supported** — `ThrowExpression` | **Supported** — exceptional transfer to handler or exit | **Not supported yet** |
| Ordinary inline assembly | **Supported** — `AsmStatement` | **Shadowed** — atomic operation | **Not supported yet** |
| `asm goto` | **Supported** — `AsmStatement` with target metadata | **Supported** — label branches and fallthrough | **Not supported yet** |
| Coroutine body | **Supported** — `CoroutineBodyStatement` | **Shadowed** — high-level coroutine body, not generated state machine | **Not supported yet** |
| `co_return` | **Supported** — `CoReturnStatement` | **Supported** — value evaluation and transfer to exit | **Not supported yet** |
| `co_await` and `co_yield` | **Supported** — `CoAwaitExpression`, `CoYieldExpression` | **Shadowed** — high-level ready, suspend and resume flow | **Not supported yet** |
| Other traversed non-expression statements | **Shadowed** — `OtherStatement` with concrete Clang kind | **Shadowed** — transparent child flow or atomic operation | **Not supported yet** |
| Lambda expressions and captures | **Supported** — `LambdaExpression`; body retained in enclosing AST | **Supported** — captures execute; lambda body is opaque in enclosing CFG | **Not supported yet** |
| Function, member and overloaded-operator calls | **Supported** — `CallExpression` | **Shadowed** — atomic intraprocedural call with conservative exceptional flow | **Not supported yet** |
| Built-in binary operators, including assignment, comma, `&&` and `||` | **Supported** — `BinaryExpression` | **Supported** — sequencing and short-circuit flow where required | **Not supported yet** |
| Built-in unary operators | **Supported** — `UnaryExpression` | **Supported** for `!`; otherwise **Shadowed** as an atomic expression | **Not supported yet** |
| Conditional expressions (`?:` and GNU omitted-middle form) | **Supported** — `ConditionalExpression` | **Supported** — conditional value flow | **Not supported yet** |
| GNU `__builtin_choose_expr` | **Supported** — `CompileTimeConditionalExpression` | **Supported** — selected or dependent branch flow | **Not supported yet** |
| Declaration references and member access | **Supported** — `DeclarationReferenceExpression`, `MemberExpression` | **Shadowed** — atomic value operations | **Not supported yet** |
| Explicit and implicit casts | **Supported** — `CastExpression` | **Shadowed** — atomic or transparent around control-relevant children | **Not supported yet** |
| Integer, floating, character, string, Boolean and null-pointer literals | **Supported** — `LiteralExpression` | **Shadowed** — atomic values | **Not supported yet** |
| Construction, `new`, `delete`, `this`, subscripting, initializer lists, `sizeof`, `noexcept`, `typeid`, `requires`, pack and trait expressions | **Shadowed** — `OtherExpression` with concrete Clang kind and semantic metadata | **Shadowed** — atomic or transparent; unevaluated children do not execute | **Not supported yet** |
| Any other traversed Clang expression | **Shadowed** — `OtherExpression` with concrete Clang kind | **Shadowed** — atomic operation or preserved child control flow | **Not supported yet** |
| Implicit expressions | **Supported** — retained under their explicit or fallback expression kind | **Supported / Shadowed** according to their project kind | **Not supported yet** |
| Implicit declarations | **Not supported** as nodes; relevant descendants are still traversed | Not applicable | **Not supported yet** |
| Declarations without definitions | **Not supported** as function graphs | Not applicable | **Not supported yet** |
| Header-only definitions, implicit template instantiations and duplicate function identities | **Not supported** as separately collected function graphs | Not applicable | **Not supported yet** |
| Lambda call operators as separate functions | **Not supported** as separate graphs; lambda body stays in its enclosing AST | Not applicable | **Not supported yet** |
| Translation-unit structure outside collected function definitions | **Not supported** | Not applicable | **Not supported yet** |
| Types, template arguments and complete template structure as independent nodes | **Not supported**; selected type text and template semantics may be metadata | Not applicable | **Not supported yet** |
| Attributes, comments and preprocessor directives as independent nodes | **Not supported**; selected attributes such as `noreturn` may become metadata | Not applicable | **Not supported yet** |
| Name lookup, overload sets and symbol-reference edges | **Not supported** | Not applicable | **Not supported yet** |

Support must not disappear downstream: every valid `FunctionAstGraph` must
produce a `FunctionCfgGraph`; once CDG is implemented, every valid
`FunctionCfgGraph` must produce a `FunctionCdgGraph`. A downstream graph may
omit syntax irrelevant to its analysis, but it must not reject an otherwise
valid upstream graph. Invalid source code and malformed internal graphs are
negative inputs rather than coverage categories.

## License

Licensed under the [Apache License 2.0](LICENSE).
