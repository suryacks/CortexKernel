# Contradiction detection

This is the one genuinely novel piece of CortexKernel. Everything else in
the platform (storage engine, caching, gateway, orchestration) is solid
engineering execution around this idea, not claimed as novel itself.

## The typed-edge distinction

Every edge in the graph is tagged with an `edge_class`. Two of those
classes are the interesting ones:

- **`StatedValue`** — what someone says they believe, want, or prioritize.
- **`BehaviorEvidence`** — what they actually did.

Comparing edges of the *same* class that conflict is a plain factual
contradiction. Comparing a `StatedValue` edge against a `BehaviorEvidence`
edge that conflicts is something more specific: a gap between what
someone claims and what they do. Keeping these as distinct, typed
classes (rather than one generic "fact" type) is what makes both kinds
of detection possible from the same underlying data. This comparison
was checked against existing memory/knowledge-graph systems (Graphiti,
Mem0, MnemeBrain) — none of them make this specific typed comparison.

## Three detectors, one module

`ContradictionDetector` (`services/storage/include/contradiction_detector.hpp`)
wraps a `GraphStore` and exposes three operations:

### `find_direct_contradictions()`

Groups live (non-invalidated) edges by `(subject_id, predicate)`, then
within each `edge_class`, finds distinct `object_id` values. Two live
edges with the same subject and predicate but different objects, of the
same class, are a direct contradiction — e.g. `self --lives_in--> nyc`
and `self --lives_in--> la` both live at once.

### `find_value_behavior_mismatches()`

Same grouping, but specifically pairs a live `StatedValue` edge against
a live `BehaviorEvidence` edge with a different object — e.g.
`self --prioritizes--> health` (StatedValue) versus
`self --prioritizes--> work` (BehaviorEvidence).

### `classify_drift(subject_id, predicate)`

Given the full history (live and invalidated) for one subject+predicate
pair, classifies how that belief has evolved into one of five states:

| State | Meaning |
|---|---|
| `HELD` | Exactly one edge, still live, no history of change. |
| `REFINED` | Multiple edges over time, but each earlier one was cleanly invalidated before the next — a normal, acknowledged evolution. |
| `CONTRADICTED` | Two or more live edges of the same class disagree right now, unresolved. |
| `BOTH` | Two or more live edges disagree *and* span both `StatedValue` and `BehaviorEvidence` — a live value/behavior mismatch. |
| `SUPERSEDED` | The most recent edge was invalidated and nothing live replaced it — the belief was retracted, not replaced. |

## A performance note, not just a correctness one

The first implementation of `find_direct_contradictions()` and
`find_value_behavior_mismatches()` compared every pair of edges within a
group — O(n²) per group. A benchmark (`bench/contradiction_bench.cpp`)
found this took **~381ms at 10,000 edges** (20 predicates). The fix:
bucket live edges by `object_id` within each group first (a single
pass), then only compare across distinct buckets. Same output, **~8.0ms**
at the same scale — roughly **47x faster**. See
[BENCHMARKS.md](./BENCHMARKS.md) for the full numbers and how to
reproduce them. The takeaway: getting the typed-edge idea right was the
interesting design problem; making the algorithm scale was a separate,
equally real engineering problem, and both are documented here rather
than only the flattering one.

## What this doesn't do yet

Detection today is exact-match: `object_id` has to be identical for two
edges to be compared. "I value my health" and "I value staying healthy"
wouldn't be linked. `SemanticIndex` (embedding + cosine similarity) exists
as a first step toward fixing that, but isn't wired into the detector
yet — see the roadmap in [`CLAUDE.md`](../CLAUDE.md).
