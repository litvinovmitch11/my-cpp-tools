# my-cpp-tools

`my-cpp-tools` builds visual graphs from C++ source code. The alpha pipeline is:

```text
editor -> Go API -> AstPrinter -> DOT -> Graphviz Worker -> SVG
```

## Prerequisites

- CMake 3.28 or newer;
- LLVM and Clang development packages;
- Go 1.27 or newer;
- Node.js 24 or newer.

## Run locally

Build the AST tool and start the backend:

```bash
cmake -S cpp -B cpp/build
cmake --build cpp/build --target AstPrinter

cd backend
AST_PRINTER_PATH=../cpp/build/src/tools/AstPrinter/AstPrinter go run ./cmd/server
```

In another terminal, start the web application:

```bash
cd web
npm ci
npm run dev
```

Open `http://127.0.0.1:5173`. Vite proxies `/api` requests to the backend on
`127.0.0.1:8080`.

## End-to-end tests

The end-to-end test builds `AstPrinter`, starts the backend and web application,
then verifies the complete pipeline in Chromium:

```bash
./scripts/e2e.sh
```

The script installs the Chromium version pinned by Playwright when needed.
