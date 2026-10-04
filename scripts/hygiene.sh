#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

cd "${repository_root}"

if rg --hidden --pcre2 '[\x{0400}-\x{04FF}]' \
    --glob '!.git/**' \
    --glob '!cpp/build*/**' \
    --glob '!web/node_modules/**' \
    --glob '!web/dist/**' \
    --glob '!web/coverage/**' \
    --glob '!web/playwright-report/**' \
    --glob '!web/test-results/**'; then
    echo "Cyrillic text is not allowed in repository sources or documentation."
    exit 1
fi

ignored_tracked="$(git ls-files --cached --ignored --exclude-standard)"
if [[ -n "${ignored_tracked}" ]]; then
    echo "Generated or local files are tracked:"
    echo "${ignored_tracked}"
    exit 1
fi

echo "Repository hygiene checks passed."
