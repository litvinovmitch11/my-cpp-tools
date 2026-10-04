#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ast_printer="${AST_PRINTER_PATH:-${repository_root}/cpp/build/src/tools/AstPrinter/AstPrinter}"
export GOCACHE="${GOCACHE:-${repository_root}/.cache/go-build}"
processes=()

cleanup() {
    local process
    for process in "${processes[@]}"; do
        kill "${process}" 2>/dev/null || true
    done
    wait "${processes[@]}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

cmake -S "${repository_root}/cpp" -B "${repository_root}/cpp/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${repository_root}/cpp/build" --target AstPrinter --parallel

(
    cd "${repository_root}/backend"
    AST_PRINTER_PATH="${ast_printer}" go run ./cmd/server
) &
processes+=("$!")

(
    cd "${repository_root}/web"
    npm run dev
) &
processes+=("$!")

echo "Web: http://127.0.0.1:5173"
echo "API: http://127.0.0.1:8080"
wait -n "${processes[@]}"
