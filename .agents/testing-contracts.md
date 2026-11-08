# Analysis support and test contracts

## Status vocabulary

Every analysis claim uses exactly one status:

- **Supported**: the behavior has an explicit semantic contract and regression
  evidence. For compiler-adapted behavior this normally means both a focused
  project-model unit test and an end-to-end golden example.
- **Abstracted**: valid input is accepted and produces a graph, but the project
  intentionally guarantees a source-oriented or conservative model rather
  than complete compiler lowering. The abstraction and its evidence must be
  documented.
- **Not implemented**: the project makes no analysis claim yet. This is for
  missing analysis stages or capabilities, not for valid syntax that can be
  represented atomically.

Invalid C++ input and malformed internal graphs are **negative inputs**, not a
fourth support status.

The README construct matrix uses **Shadowed** for the construct-level instance
of Abstracted behavior: the construct is preserved through a generic node,
atomic operation, transparent wrapper, or documented high-level lowering.

## Required evidence

When adding or changing Supported behavior:

1. State the project-owned semantic contract in the relevant model document.
2. Add or update a focused unit test that asserts nodes, edges, ordering,
   reachability, or the typed error rather than only a DOT string.
3. If Clang mapping or the native tool path is involved, add the construct to
   `cpp/examples` and review both AST and CFG goldens.
4. For a bug, make the regression fail before the fix when practical.
5. Keep the support matrix in `docs/analysis-support.md` consistent.

When adding or changing Abstracted behavior:

1. Describe what is preserved and what is deliberately omitted.
2. Test the promised abstraction, not an accidental compiler implementation
   detail.
3. Do not label it unsupported merely because it is conservative.

Not implemented capabilities must not appear as working in README, model docs,
API contracts, examples, or UI. Do not add permanently failing or disabled
tests as placeholders.

## Test layers

- `unit;contract`: domain invariants and AST-to-CFG semantics using only
  project-owned models.
- `integration;golden`: Clang adapter, native tool, deterministic presentation,
  and escaping over the shared example corpus.
- `integration;negative`: invalid source, diagnostics, non-zero status, and no
  partial output.
- `architecture;contract`: dependency-boundary and forbidden-coupling guards.

Useful focused commands:

```bash
ctest --test-dir cpp/build -L contract --output-on-failure
ctest --test-dir cpp/build -L golden --output-on-failure
ctest --test-dir cpp/build -L negative --output-on-failure
ctest --test-dir cpp/build -L architecture --output-on-failure
```

The full CTest run remains mandatory before completion.
