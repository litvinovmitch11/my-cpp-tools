# Agent workflow and validation

## Language and naming

- Use English for code, comments, documentation, UI text, diagnostics, test
  names, commit messages, and GitHub metadata.
- Prefer domain names over generic names such as `data`, `item`, or `manager`.
- Use AST, CFG, and CDG consistently in prose and each language's established
  casing in identifiers.
- Name tests after observable behavior.
- Do not add a TODO without an owner-facing explanation or tracked issue.

## Validation commands

Run focused checks during iteration and all relevant completion checks. Run the
full pipeline for cross-component changes.

### C++

```bash
cmake -S cpp -B cpp/build
cmake --build cpp/build
ctest --test-dir cpp/build --output-on-failure
cmake --build cpp/build --target format-check
cmake --build cpp/build --target tidy
./scripts/cpp-coverage.sh
```

Review an intentional graph change before updating goldens:

```bash
cmake --build cpp/build --target update-ast-goldens
cmake --build cpp/build --target update-cfg-goldens
```

For native architecture changes, also validate a release configuration with
`BUILD_TESTING=OFF` so deployable targets do not accidentally depend on tests.

### Go

```bash
cd backend
go test ./...
go test -race ./...
go vet ./...
```

Native-process integration tests require `AST_PRINTER_PATH`.

### Web

```bash
cd web
npm ci
npm run check
npm run e2e
```

### Full pipeline

```bash
./scripts/e2e.sh
```

## Testing expectations

- Add unit tests for new domain logic and error mappings.
- Add integration tests at native-process and HTTP boundaries.
- Add or update E2E tests for visible cross-component behavior.
- Add a regression test for every bug fix when practical.
- Test limits and failures, not only happy paths.
- Coverage thresholds are gates, not a reason to test trivial lines or
  implementation details.
- Golden files are reviewed expected behavior, never an automatic substitute
  for understanding a diff.
- Use the Supported, Abstracted, and Not implemented statuses from
  `testing-contracts.md`; do not call a conservative valid CFG unsupported.

## Maintainability and spaghetti-code guard

- Treat growing cognitive complexity as an architectural signal, not merely a
  style concern. Correct code with green tests can still become unsafe to
  extend when one change requires coordinated edits across several dispatchers
  or hidden state machines.
- Do not let a builder, mapper, handler, service, or component accumulate
  unrelated responsibilities. Preserve a clear transformation or ownership
  boundary for every class and module.
- Watch specifically for duplicated semantic dispatch, long functions with
  nested branching, manual push/pop context stacks, pointers to temporary
  context objects, implicit cross-method state, cyclic dependencies, and files
  that can only be understood by reading them end to end.
- When one of these risks appears, address it before adding more behavior in
  the affected area. Prefer small cohesive helpers, explicit value objects,
  scoped context guards, and private implementation boundaries. Do not replace
  straightforward code with factories, inheritance, registries, or template
  machinery merely to reduce file length.
- Refactoring must preserve domain contracts and be protected by focused unit
  tests and reviewed goldens. Separate structural cleanup from semantic changes
  when practical.
- Do not perform speculative repository-wide rewrites. Refactor the smallest
  hotspot whose current complexity makes the next intended change unsafe.

## Security boundaries

- Never commit secrets, tokens, keys, credentials, or real `.env` files.
- Do not log environment variables or secret-bearing configuration.
- Keep GitHub Actions permissions minimal and pin third-party actions.
- Treat Clang as a native parser of hostile input. Timeouts and output limits
  do not provide filesystem or memory isolation.
- Keep the alpha backend loopback-only. Public deployment requires process and
  filesystem isolation, memory and CPU limits, and bounded concurrency.
- The backend must never compile or execute submitted programs and must
  preserve request, timeout, diagnostic, output, and concurrency limits.
- The web must keep worker-based Graphviz layout, SVG sanitization, request
  cancellation, stale-response protection, keyboard behavior, and accessible
  labels.

## Change workflow

1. Inspect relevant code, tests, task-specific agent contracts, and current
   diff; avoid unrelated files by default.
2. State assumptions when a decision changes an API or semantic contract.
3. Implement the smallest coherent change.
4. Check whether the change increases one of the spaghetti-code risks above;
   if it does, reduce that risk before extending the hotspot further.
5. Run focused checks, then all required completion checks.
6. Review the final diff for generated files, Russian text, secrets, internal
   path leaks, accidental dependencies, and unrelated changes.
7. Report limitations explicitly instead of hiding them behind green tests.

Do not commit, push, merge, publish, change repository settings, or create a
release without explicit owner authorization.

## Multiple agents

The primary agent owns the plan, architecture, shared-file edits, integration,
and final verification. Delegate only independent bounded workstreams, provide
acceptance criteria and relevant contracts, and avoid concurrent edits to the
same files. The primary agent reconciles findings and runs integration checks.

Use the repository, this directory, the current diff, and a concise brief as
shared context. Do not copy long conversations into subagent prompts.

## Definition of done

A change is complete when:

- requested behavior and architectural boundaries match;
- relevant formatters, linters, builds, tests, and coverage pass;
- cross-component work passes the full end-to-end pipeline;
- important success and failure behavior is tested;
- public contracts and documentation are updated;
- no secrets, generated artifacts, Russian text, or unrelated changes remain;
- security and operational limitations are stated clearly.
