# CDG model and semantics

The native CDG stage transforms each project-owned `FunctionCfgGraph` into a
project-owned `FunctionCdgGraph`. It does not use `clang::CFG`, LLVM analysis
passes, compiler control-dependence output, or a third-party graph builder.

The browser, HTTP API, DOT serializer, and `CdgPrinter` do not expose CDG yet.
This document describes the native analysis contract that those layers will
consume.

## Function and node coverage

Every valid `FunctionCfgGraph` produces a `FunctionCdgGraph`. Construction does
not reject a CFG because it contains unreachable code, exceptional flow,
indirect branches, assembly branches, no-return operations, or paths that may
execute forever.

The CDG contains exactly one node for every CFG node, in CFG insertion order.
Each node owns:

- its `CfgNodeId` origin;
- a CDG kind mapped from the CFG kind;
- the CFG display label and optional source range;
- whether the CFG node is reachable from the function entry.

The kind mapping is:

| CFG kind | CDG kind |
| --- | --- |
| `Entry` | `Entry` |
| `Exit` | `Exit` |
| `Operation` | `Operation` |
| `Condition` | `Predicate` |
| `Transfer` | `Transfer` |
| `Anchor` | `Anchor` |

The CDG root is the node originating from the CFG entry. Unreachable CFG nodes
remain in the CDG with `ReachableFromEntry == false`; control dependence inside
an unreachable component is still computed from that component's structure.

## Strong post-dominance

The implementation uses strong, termination-sensitive post-dominance. A CFG
node `D` post-dominates a node `N` when every maximal path beginning at `N`
visits `D`. A maximal path may be:

- a finite path ending at the function exit;
- a finite path ending at another terminal node, such as a no-return
  operation;
- an infinite path through a cycle.

Post-dominance is reflexive. A terminal node therefore post-dominates itself,
but the ordinary function exit does not post-dominate a branch that can instead
reach a no-return terminal or execute forever.

The relation is computed directly as a least fixed point of mandatory
reachability. No synthetic edge connects no-return or non-terminating regions
to the function exit. This avoids claiming termination that the CFG does not
guarantee.

`PostDominatorRelation` also reports two separate existential properties:

- `canReachFunctionExit(N)` is true when at least one path from `N` reaches the
  CFG exit;
- `mayDiverge(N)` is true when at least one infinite CFG path begins at `N`.

These properties document termination behavior but do not change the
post-dominator relation. `immediatePostDominator(N)` is returned only when the
closest strict post-dominator is unique. It may be absent for terminal nodes
and non-terminating regions.

## Control dependence

For each CFG edge `A -> S`, the builder first ignores the edge when `S`
post-dominates `A`; such an edge does not represent a control choice. Otherwise
a CFG node `X` is control-dependent on that edge when:

1. `X` post-dominates `S`; and
2. `X` is `A`, or `X` does not post-dominate `A`.

The CDG then contains an edge from the CDG node for `A` to the CDG node for
`X`. Loop predicates may therefore have a self-dependence edge: taking the loop
branch controls whether the predicate executes again.

There is one CDG edge for each controlling CFG edge and dependent node. The CDG
edge retains:

- the originating `CfgEdgeId`;
- the complete `CfgEdgeKind`, including true, false, case, default, exception,
  indirect, assembly, fallthrough, suspend, and resume edges;
- the optional CFG edge label;
- whether the dependency is ordinary or exceptional control.

Potentially throwing operations can act as controllers because normal and
exceptional successors are alternative control outcomes. Indirect goto and
`asm goto` targets are handled the same way as structured branches.

## Determinism and complexity

Nodes follow CFG node order. Control-dependence edges follow CFG edge order and
then dependent CFG node order. Results are deterministic for the same CFG.

For a CFG with `V` nodes and `E` edges, mandatory reachability is computed once
per candidate post-dominator in `O(V * (V + E))` time and `O(V^2)` relation
storage. CDG construction then checks each CFG edge against every CFG node in
`O(E * V)` time. This deliberately favors a small, auditable implementation;
an individual immediate-post-dominator lookup is `O(V^2)` and is not needed by
CDG construction. The API can later preserve the same semantics behind a more
compact relation representation if profiling proves it necessary.

## Current integration boundary

The native model, post-dominator relation, and CFG-to-CDG builder are complete.
The next stage is presentation and product integration:

1. deterministic CDG DOT serialization;
2. a real `CdgPrinter` composition root;
3. bounded backend routing;
4. browser selection, rendering, interaction, and end-to-end tests.
