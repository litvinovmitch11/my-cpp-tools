#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export GOCACHE="${GOCACHE:-${repository_root}/.cache/go-build}"

cmake -S "${repository_root}/cpp" -B "${repository_root}/cpp/build"
cmake --build "${repository_root}/cpp/build" --target AstPrinter

npm --prefix "${repository_root}/web" ci

cd "${repository_root}/web"
if [[ "${CI:-}" == "true" ]]; then
    npm exec playwright install --with-deps chromium
else
    npm exec playwright install chromium
fi
npm run e2e
