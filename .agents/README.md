# Agent documentation

This directory is the durable engineering context for automated contributors.
It complements the concise rules in the repository-level [`AGENTS.md`](../AGENTS.md)
and is intentionally separate from the human-facing documentation in `docs/`.

## Reading order

For every task:

1. Read [`AGENTS.md`](../AGENTS.md).
2. Read [`architecture.md`](architecture.md) for component ownership, current
   status, dependency direction, decision history, and roadmap.
3. For native graph work, read [`cpp-contracts.md`](cpp-contracts.md) and the
   relevant human semantic reference:
   [`docs/ast-model.md`](../docs/ast-model.md) or
   [`docs/cfg-model.md`](../docs/cfg-model.md).
4. Read [`workflow.md`](workflow.md) before validation or repository-wide
   changes.
5. Read [`testing-contracts.md`](testing-contracts.md) before changing a
   supported analysis claim or its evidence.
6. Read [`backend-contracts.md`](backend-contracts.md) before changing the Go
   API, native-process orchestration, or the C++/Go process boundary.
7. Read [`web-contracts.md`](web-contracts.md) before changing browser API
   access, graph state, rendering, interaction, or accessibility.
8. Inspect `git status`, the current branch diff, relevant public headers,
   target links, and focused tests. Documentation records contracts; source and
   tests record the current implementation.

## Document ownership

- `architecture.md` records durable project context and staged plans.
- `cpp-contracts.md` records exact compiler, ownership, AST, CFG, and
  presentation boundaries.
- `workflow.md` records validation, testing, security, multi-agent, and
  completion rules.
- `testing-contracts.md` defines support statuses and the evidence required for
  each status.
- `backend-contracts.md` records the stable API, process, error, resource, and
  current backend-milestone contracts.
- `web-contracts.md` records browser ownership, analysis selection, request,
  rendering, sanitization, stale-result, and accessibility contracts.
- `README.md` and `docs/` are written for people using or reviewing the
  project. Keep them concise and explanatory rather than agent-operational.

Temporary implementation plans, unresolved discussion, and raw conversation
history stay in the active task context. Promote only stable decisions or
contracts into this directory.
