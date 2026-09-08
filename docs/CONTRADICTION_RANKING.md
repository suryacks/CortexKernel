# Closing the loop: detection → ranking → feedback

Every earlier round of this project built one piece of a pipeline
without connecting it to the others: storage could detect
contradictions, and a bandit could rank things by learned preference,
but nothing actually called the bandit with real contradictions. This
is that wiring, done for real and verified end-to-end with three live
processes.

## The flow

```
storage:/contradictions  →  extraction:/contradictions/ranked  →  ranking:/rank
                                                                         ↑
                                          extraction:/feedback  →  ranking:/feedback
```

1. `GET extraction:/contradictions/ranked` calls storage's
   `GET /contradictions`, converts each result into the bandit's
   `{id, category, confidence}` shape (`ranking_client.contradiction_to_rank_item`),
   POSTs the list to ranking's `/rank`, and returns the original
   contradiction objects re-ordered by the bandit's response.
2. `POST extraction:/feedback` (`{"category": "...", "reward": ...}`)
   forwards straight to ranking's `/feedback`, which updates that
   category's learned value and persists it to `bandit_state.json`.

## Verified, not assumed

Ran all three services as real processes (`storage_server`, `ranking/service.py`,
`extraction/app.py`) and drove them with curl:

1. Seeded a direct contradiction (`self --lives_in--> nyc` vs `la`) and
   a value/behavior one (`self --prioritizes--> work` StatedValue vs
   `self --prioritizes--> rest` BehaviorEvidence).
2. `GET /contradictions/ranked` returned both, in whatever order the
   bandit defaults to before any feedback.
3. `POST /feedback {"category": "value_behavior", "reward": 1.0}` and
   `POST /feedback {"category": "direct", "reward": 0.0}`.
4. `GET /contradictions/ranked` again — **the order actually flipped**,
   `value_behavior` now first. Not inferred from reading the bandit's
   code; observed in the actual HTTP response.

## The frontend is wired into this loop too

`App.tsx` now calls `fetchRankedContradictions()` (extraction's
`/contradictions/ranked`) instead of storage's raw `/contradictions`,
and each item in `ContradictionsPanel` gets a `FeedbackButtons`
component ("Useful" / "Dismiss") that calls `submitFeedback()`
(extraction's `POST /feedback`) and re-fetches the ranked list on
success — so clicking a button in the UI actually updates the bandit
and the displayed order, end to end. Verified via a clean
`tsc -b && vite build` (no headless browser was available to click the
actual buttons and watch the DOM reorder — see the same caveat in
`docs/DEPLOYMENT.md`/`CLAUDE.md` about the frontend generally). The
backend half of this exact flow (`/contradictions/ranked` reordering
after `/feedback`) *was* verified with real curl calls against three
live processes — see above.

## What's still not connected

- Nothing calls `/contradictions/semantic` (see below) from this loop
  yet — only the exact-match `/contradictions` feeds into ranking.

## A second, related piece built this round: semantic value/behavior detection

`ContradictionDetector::find_semantic_value_behavior_mismatches()` is
new: instead of requiring an exact `predicate` + `object_id` match, it
embeds `predicate + " " + object_name` for every live `StatedValue` and
`BehaviorEvidence` edge belonging to the same subject (via the existing
placeholder `embed_text()`/`SemanticIndex` machinery) and flags pairs
above a cosine-similarity threshold — regardless of whether their
predicates or object IDs match at all. Exposed at
`GET /contradictions/semantic?threshold=0.5`.

**Be precise about what this proves and what it doesn't.** `embed_text()`
is still a hashed-trigram character bag, not a real embedding model — it
catches near-identical *text* (e.g. two edges whose object nodes are
both literally named "personal health"), not genuine semantic
equivalence (it will not link "my health" and "staying healthy", which
share almost no character trigrams). The verified test cases in
`test_semantic_contradictions.cpp` reflect this honestly: they test
text-similarity flagging, not semantic understanding. Real semantic
matching is still blocked on swapping in a real embedding model or API
(see `CLAUDE.md`, item 13) — this round only wired the *machinery* that
a real embedding would slot into.
