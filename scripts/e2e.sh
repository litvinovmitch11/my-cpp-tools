#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export GOCACHE="${GOCACHE:-${repository_root}/.cache/go-build}"
ast_printer="${repository_root}/cpp/build/src/tools/AstPrinter/AstPrinter"
cfg_printer="${repository_root}/cpp/build/src/tools/CfgPrinter/CfgPrinter"
cdg_printer="${repository_root}/cpp/build/src/tools/CdgPrinter/CdgPrinter"

cmake -S "${repository_root}/cpp" -B "${repository_root}/cpp/build"
cmake --build "${repository_root}/cpp/build" --target AstPrinter CfgPrinter CdgPrinter

(
    cd "${repository_root}/backend"
    AST_PRINTER_PATH="${ast_printer}" \
        CFG_PRINTER_PATH="${cfg_printer}" \
        CDG_PRINTER_PATH="${cdg_printer}" \
        go test ./internal/api -run Integration -count=1
)

npm --prefix "${repository_root}/web" ci

cd "${repository_root}/web"
if [[ "${CI:-}" == "true" ]]; then
    npm exec playwright install --with-deps chromium
else
    npm exec playwright install chromium
fi
npm run e2e
