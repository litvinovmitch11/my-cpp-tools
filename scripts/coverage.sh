#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

"${repository_root}/scripts/go-coverage.sh"
"${repository_root}/scripts/cpp-coverage.sh"
npm --prefix "${repository_root}/web" run test:coverage
