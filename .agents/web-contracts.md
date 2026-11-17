# Web contracts

## Responsibility boundary

The browser owns source editing, analysis selection, HTTP requests, Graphviz
layout, sanitized SVG display, interaction, and accessibility. It does not
parse DOT, infer AST or CFG semantics from labels or shapes, or reproduce
native analysis rules.

The supported browser analysis kinds are a closed project-owned set: `ast` and
`cfg`. AST remains the default. Each kind maps to its explicit versioned API
route; arbitrary strings must not be interpolated into API paths.

## Selection and request lifecycle

- The user explicitly selects AST or CFG and explicitly starts a build.
- Changing the selected kind does not automatically run analysis.
- Changing the selected kind clears the previous result and error so a graph
  is never displayed under the wrong analysis label.
- Analysis selection is disabled while a request or layout is active.
- Starting a new request aborts the previous request and invalidates its
  result. Unmounting the application aborts any active request.
- An HTTP response or Worker response belonging to an older request or render
  must never replace the current result.
- AST and CFG share one request/error implementation because their backend
  envelopes are identical. Kind-specific UI wording may differ.

## Rendering and trust boundary

DOT returned by the backend and SVG returned by Graphviz are untrusted data.
Graphviz layout remains inside a Web Worker. Every SVG is sanitized before it
is inserted into the DOM. Do not add main-thread layout, unsanitized previews,
manual DOT parsing, or DOM behavior derived from native node labels.

The generic graph view receives only a layout result and presentation text. It
does not receive an analysis kind and does not branch on AST or CFG semantics.
Native presentation remains the source of truth for graph labels, colors,
shapes, and edge roles.

## Interaction and accessibility

- Analysis selection uses native radio semantics with an accessible group
  label.
- The build button, pane title, progress messages, and empty state identify the
  selected analysis kind.
- `Ctrl`/`Command` + `Enter` builds the selected kind when the application is
  ready.
- Existing keyboard editing, zoom, fit, focus, status announcements, and
  accessible labels remain available for both graph kinds.
- Worker initialization failure remains recoverable without reloading the
  page.

## Evidence

Unit tests cover both endpoint mappings, structured errors, cancellation,
cross-kind stale-response protection, selection behavior, dynamic labels, and
Worker lifecycle state. Browser E2E uses the real AST and CFG native tools and
checks successful rendering plus compiler diagnostics.
