#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
coverage_file="${repository_root}/backend/coverage.out"
export GOCACHE="${GOCACHE:-${repository_root}/.cache/go-build}"

cd "${repository_root}/backend"
go test -race -covermode=atomic -coverprofile="${coverage_file}" ./...

coverage="$(go tool cover -func="${coverage_file}" | awk '/^total:/ {gsub("%", "", $3); print $3}')"
awk -v coverage="${coverage}" 'BEGIN {
    if (coverage + 0 < 70) {
        printf "Go line coverage %.1f%% is below 70%%\n", coverage
        exit 1
    }
    printf "Go line coverage: %.1f%%\n", coverage
}'
