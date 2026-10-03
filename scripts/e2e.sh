#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "${repository_root}/cpp" -B "${repository_root}/cpp/build"
cmake --build "${repository_root}/cpp/build" --target AstPrinter

npm --prefix "${repository_root}/web" ci

cd "${repository_root}/web"
npm exec playwright install chromium
npm run e2e
