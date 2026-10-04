#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
unformatted="$(gofmt -l "${repository_root}/backend")"
if [[ -n "${unformatted}" ]]; then
    echo "Go files need formatting:"
    echo "${unformatted}"
    exit 1
fi
