# Backend contracts

## Responsibility boundary

The Go backend validates versioned HTTP requests, runs native analysis tools
under common resource bounds, and maps native outcomes to safe API errors. It
does not parse C++ itself, reconstruct graph semantics, inspect DOT contents,
or duplicate AST and CFG algorithms.

The backend invokes project tools only to parse and analyze submitted source.
It never compiles submitted source into an executable and never runs submitted
programs. Native parsing is bounded but is not an operating-system sandbox, so
the service remains loopback-only.

## Versioned graph API

The v1 graph endpoints are explicit:

- `POST /api/v1/ast` builds an AST DOT graph;
- `POST /api/v1/cfg` builds a CFG DOT graph.

Both accept the same `{ "code": string }` request and return the same
`{ "dot": string }` success envelope. They share request validation and safe
error mapping, but remain separate routes so graph types can evolve without a
request discriminator or a breaking change to the existing AST client.

Clients should send `Content-Type: application/json`; a present non-JSON or
malformed media type is rejected. A missing header remains accepted for v1 AST
compatibility. Unknown routes, unsupported methods, validation failures, and
server failures all use the same JSON error envelope. Unsupported methods set
the standard `Allow` response header.

## Analysis orchestration

The backend domain exposes an explicit closed set of analysis kinds and one
builder operation. A single runner owns the validated paths to `AstPrinter` and
`CfgPrinter`, a shared timeout, shared output and diagnostic limits, and one
shared concurrency pool. Adding another executable must not multiply the total
number of native processes that the service may run concurrently.

The runner uses a private temporary directory, writes a single `input.cpp`,
sets a minimal child environment, replaces internal paths in diagnostics,
discards partial output on failure, and removes temporary state after every
request. Source, graph, diagnostics, execution time, and total native-process
concurrency remain bounded.

## Native exit contract

Native graph tools use these process outcomes:

- exit code `0`: graph generation succeeded and stdout contains complete DOT;
- exit code `2`: Clang rejected the submitted source; stderr may contain
  diagnostics safe for sanitization and inclusion in `invalid_source`;
- any other non-zero exit code, signal, or process-start failure: internal tool
  failure, never a user source error.

Go may expose sanitized diagnostics only for exit code `2`. Internal failures
are logged without submitted source and returned as a generic `internal_error`.
Timeout, cancellation, capacity, and output-limit failures retain their own
typed outcomes.

## Public error mapping

| Outcome | HTTP status | API code |
| --- | ---: | --- |
| Unknown route | 404 | `not_found` |
| Unsupported method | 405 | `method_not_allowed` |
| Explicit non-JSON content type | 415 | `unsupported_media_type` |
| Invalid JSON or empty code | 400 | `invalid_request` |
| Request body too large | 413 | `request_too_large` |
| Source code too large | 413 | `source_too_large` |
| Native source rejection | 422 | `invalid_source` |
| DOT output too large | 422 | `graph_too_large` |
| Shared runner capacity exhausted | 503 | `builder_busy` |
| Native analysis timeout | 504 | `builder_timeout` |
| Tool start, protocol, or analysis failure | 500 | `internal_error` |

Unexpected handler panics are recovered at the HTTP boundary, logged with the
request ID and stack, and returned as a generic `internal_error` without the
panic value.

Responses never expose executable paths, temporary paths, environment values,
Go errors, or internal native diagnostics.

Every request completion is logged with request ID, method, path, status,
response size, and duration. Analysis failures add the analysis kind and safe
internal error. Submitted source and compiler diagnostics are never logged.

## CFG backend milestone

The backend CFG MR is complete only when:

1. the existing AST API remains compatible;
2. `/api/v1/cfg` returns DOT produced by the project `CfgPrinter`;
3. AST and CFG use one generic handler and one bounded runner without copied
   orchestration logic;
4. the native exit contract distinguishes invalid source from an internal CFG
   failure;
5. unit tests cover both kinds and shared cross-kind capacity;
6. integration tests run both real native tools;
7. CI builds both tools and makes those integration tests mandatory;
8. environment examples, development scripts, README, and architecture docs
   describe the new backend capability;
9. the web application remains AST-only until its separate milestone.
