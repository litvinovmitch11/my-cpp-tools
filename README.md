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

- `cpp/` contains the Clang-based `AstPrinter` and future project-owned graph
  analysis tools.
- `backend/` contains the Go API, validation, process limits, and error mapping.
- `web/` contains the React/TypeScript editor and Graphviz visualization.

CFG and CDG are deliberately outside the alpha scope. They will be implemented
manually: CFG from Clang AST data and CDG from the project-owned CFG. The project
will not use `clang::CFG`, LLVM analysis passes, or third-party CFG/CDG builders.

See [AST graph model and coverage](docs/ast-model.md) for the exact function
selection rules, node and edge categories, tested constructs, and known gaps.

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
  cpp/examples/ast/control_flow.cpp -- -std=c++20
```

Golden tests never modify their expected output. After an intentional AST or
presentation change, inspect the failing diff and explicitly update all AST
goldens with:

```bash
cmake --build cpp/build --target update-ast-goldens
git diff -- cpp/tests/golden/ast
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
- manual CFG construction from AST;
- manual CDG construction from the project-owned CFG;
- additional graph interactions and incremental analysis;
- sandboxed packaging and deployment only after explicit isolation work.

Read the project constraints in [AGENTS.md](AGENTS.md) before changing analysis
logic.

## License

Licensed under the [Apache License 2.0](LICENSE).
