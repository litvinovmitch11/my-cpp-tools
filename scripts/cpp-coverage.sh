#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${repository_root}/cpp/build-coverage"

cmake \
    -S "${repository_root}/cpp" \
    -B "${build_directory}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMYCPPTOOLS_ENABLE_COVERAGE=ON \
    -DCMAKE_CXX_COMPILER="${CXX:-clang++}"
cmake --build "${build_directory}" --target coverage --parallel
