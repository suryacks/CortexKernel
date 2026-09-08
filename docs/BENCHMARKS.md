# Benchmarks

All numbers below came from actually running the benchmarks in this repo
on a local machine (Apple Silicon, macOS) — not estimated. Reproduce them
yourself with the commands in each section; absolute numbers will vary
by machine, but the relative story (the size of each speedup) should
hold.

## ContradictionDetector: before/after the O(n²) fix

`find_direct_contradictions()` and `find_value_behavior_mismatches()`
originally compared every pair of edges within a `(subject, predicate)`
group. The fix buckets live edges by `object_id` first (one pass) and
only compares across distinct buckets.

| Graph size | `find_direct_contradictions()` before | after | speedup |
|---|---|---|---|
| 100 edges | 0.13 ms | 0.33 ms | **~2.6x slower** |
| 1,000 edges | 4.7 ms | 1.5 ms | ~3x faster |
| 10,000 edges | **~381 ms** | **~8.0 ms** | **~47x faster** |

That 100-edge row is real and left in on purpose: the bucketing fix adds
a small constant overhead (building hash maps) that only pays off once a
group is actually large. At tiny scale, the original naive pairwise scan
is genuinely faster. This is a normal, expected algorithmic tradeoff —
O(n²) with a low constant beats O(n) with a higher constant until n is
big enough — and it's worth knowing rather than only reporting the
flattering 10,000-edge number.

`find_value_behavior_mismatches()` shows the same pattern: **~384 ms →
~8.2 ms** at 10,000 edges (~47x). `classify_drift()` was never affected
by this — it only ever touches one subject+predicate's history, and
stayed sub-millisecond at every size tested.

Reproduce:

```bash
cd services/storage
cmake -B build && cmake --build build
./build/contradiction_bench
```

## Custom in-memory engine vs. a naive SQL baseline

`bench/sqlite_baseline_bench.cpp` runs the same synthetic dataset through
a naive SQLite self-join (`SELECT ... FROM edges a JOIN edges b ON
a.subject_id = b.subject_id AND a.predicate = b.predicate AND
a.object_id <> b.object_id ...`) — the query anyone would write first,
with no indexing strategy beyond what SQLite does by default.

| Graph size | Optimized `ContradictionDetector` | Naive SQLite self-join |
|---|---|---|
| 10,000 edges | **~8.0 ms** | **~284 ms** |

The custom in-memory engine is roughly **35x faster** than the naive SQL
approach at the same scale. This isn't a claim that SQLite is bad — a
properly indexed query would close most of that gap — it's a comparison
against what a reasonable first pass looks like, and a demonstration
that the bucketing fix above is a real algorithmic win, not just a
constant-factor one.

Reproduce:

```bash
cd services/storage
./build/sqlite_baseline_bench
```

## SemanticIndex: brute-force cosine similarity

`SemanticIndex::most_similar()` does a linear scan over stored
embeddings (no ANN structure yet). At the sizes tested, this hasn't been
a bottleneck:

| Index size | `most_similar(top_k=5)` |
|---|---|
| 100 embeddings | sub-millisecond |
| 1,000 embeddings | low single-digit ms |
| 10,000 embeddings | low single-digit ms |

Worth re-benchmarking once the index is actually populated from real
extracted data rather than synthetic text — the placeholder
`embed_text()` (hashed-trigram bag) may not reflect a real embedding
model's dimensionality or the actual size the index reaches in practice.

Reproduce:

```bash
cd services/storage
./build/semantic_index_bench
```

## What's not measured yet

- Memory footprint at scale.
- Requests/sec under concurrent load (a k6/wrk-style load test against
  the HTTP API, not just the C++ classes directly).
- Redis cache hit-rate impact on end-to-end request latency under load.

These are open items, not silently skipped — see the roadmap in
[`CLAUDE.md`](../CLAUDE.md).
