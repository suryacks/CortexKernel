# CortexKernel — Project Context for Claude Code

## What this is

CortexKernel is a **distributed, bi-temporal knowledge graph platform** for
tracking a person's stated values against their observed behavior over
time — built as a portfolio/resume project demonstrating systems
programming (C++), distributed systems (gRPC, event-driven pipelines),
ML/NLP engineering (Python, LLM-based extraction) and ML infrastructure
(a self-hosted embedding + vector-similarity index feeding contradiction
detection), reinforcement learning (a bandit that learns which
contradictions are worth surfacing from real user feedback),
performance/quant-style engineering (benchmarked, profiled, and optimized
detection algorithms with published latency numbers), data infrastructure
(caching, embedded durable storage), containerization (Docker),
orchestration (Kubernetes + Helm), infra-as-code (Terraform), and CI/CD
(GitHub Actions). It is not a daily-use production tool — correctness,
clean architecture, and breadth of demonstrable engineering practice
matter more than feature completeness.

The core idea: every fact in the graph is a typed, bi-temporal edge, and
the system distinguishes **STATED-VALUE** edges (what someone says they
believe/want) from **BEHAVIOR-EVIDENCE** edges (what they actually did).
This typed-edge distinction is what makes automatic contradiction
detection and belief-drift classification possible — this is the
project's one genuinely novel technical idea (validated via research
against existing systems like Graphiti, Mem0, MnemeBrain — none of them
do this specific typed comparison). Everything else in the platform
(storage engine, service mesh, caching, orchestration) is solid,
resume-relevant engineering execution layered around that core idea.

## Owner's background and working style

Surya — rising sophomore at Cornell (CS + ORIE), learning C++ actively
through this project. Prefers:
- **No comments in code.** Code should be self-explanatory through naming
  and structure. Explanations of design decisions live in this file and
  in commit messages/PR descriptions, not as inline comments.
- Small, incremental, testable steps over large one-shot builds.
- One commit per meaningful, compiling, tested unit of work — real
  incremental commit history is itself part of the resume value.
- Ambitious project scope on paper (for resume/recruiter framing) but
  execution stays honest: the roadmap below marks exactly what's DONE vs
  PLANNED, and nothing gets claimed as working until it's tested.

## Repository

https://github.com/suryacks/CortexKernel (public). No personal name in
repo title (deliberate choice). Local folder: `~/CortexKernel`.

**Critical: never commit personal data.** `.gitignore` excludes `*.db`,
`.venv/`, `build/`, `.env`, `k8s/secret.yaml`, and any folder containing
actual personal life-context content. The code is the portfolio piece;
personal data (if ever used for real testing) stays local and private,
never pushed.

## Architecture

```
CortexKernel/
  services/
    storage/                    — C++ storage engine + JSON HTTP API (IN PROGRESS)
      include/
        graph_types.hpp          — Node, Edge, NodeType, EdgeClass (DONE)
        graph_store.hpp          — in-memory storage engine (DONE)
        contradiction_detector.hpp — direct-contradiction, value/behavior
                                      mismatch, and drift-state detectors (DONE)
        json_translation.hpp     — DTO layer, JSON <-> C++ types (DONE)
        persistence.hpp          — WAL-based durability for GraphStore (DONE, not wired into main.cpp yet)
        semantic_index.hpp       — embedding + cosine-similarity vector index (DONE, placeholder embeddings)
        httplib.h                — vendored single-header HTTP library
        json.hpp                 — vendored nlohmann/json single header
      src/
        graph_store.cpp
        contradiction_detector.cpp
        json_translation.cpp
        persistence.cpp
        semantic_index.cpp
        main.cpp                 — HTTP server entrypoint (DONE, still in-memory only)
      tests/
        test_graph_store.cpp      — Catch2 unit tests (DONE, passing)
        test_json_translation.cpp — Catch2 unit tests (DONE, passing)
        test_contradiction_detector.cpp — Catch2 unit tests (DONE, passing)
        test_semantic_index.cpp  — Catch2 unit tests (DONE, passing)
      bench/
        contradiction_bench.cpp  — latency/throughput microbenchmark for
                                    ContradictionDetector at increasing graph
                                    sizes (DONE, see Current status for findings)
      CMakeLists.txt              — FetchContent for Catch2 (DONE)
      Dockerfile                  — multi-stage build (NOT STARTED)
    extraction/                  — Python LLM-based extraction pipeline (IN PROGRESS)
      Calls the Anthropic API to turn raw journal/chat text into typed
      Node/Edge candidates, posts them to the storage service's REST API.
      Planned stack: FastAPI, Pydantic, httpx.
      ranking/
        bandit.py                — epsilon-greedy contextual bandit (RL) that
                                    ranks detected contradictions by learned
                                    category value + confidence (DONE, no
                                    caller wired up yet, no persistence of
                                    learned state across restarts)
    gateway/                     — planned: lightweight API gateway in front
      of storage + extraction, handling auth (API keys) and rate limiting.
      (PLANNED, Tier 3)
  infra/
    k8s/                         — raw Kubernetes manifests (NOT STARTED)
    helm/                        — Helm chart replacing raw manifests (PLANNED)
    terraform/                   — IaC for the `kind`/cloud cluster + any
      managed resources (Redis, object storage) (PLANNED)
  web/                            — React + TypeScript + D3.js graph
    visualization dashboard (PLANNED, Tier 4)
  .github/workflows/
    ci.yml                       — builds + runs C++ tests on every push (DONE, green)
  README.md                      — needs full writeup (NOT STARTED)
  .gitignore
```

## Tech stack (current + planned, for resume framing)

- **Systems layer (C++17)**: custom in-memory graph store, planned
  embedded durable storage (RocksDB or a hand-rolled WAL — TBD), REST API
  via a single-header HTTP library, Catch2 for unit testing, CMake +
  FetchContent for reproducible builds.
- **Inter-service communication**: REST/JSON today; planned gRPC +
  Protocol Buffers for the extraction-to-storage path once there are two
  real services (demonstrates both API styles deliberately).
- **Event-driven ingestion (planned)**: Redis Streams (or NATS) between
  the extraction service and storage, so extraction can run async and
  storage doesn't block on LLM latency.
- **Caching (planned)**: Redis in front of hot read paths (`/nodes/:id`,
  `/stats`) with explicit invalidation on writes.
- **ML/NLP (planned)**: Python extraction service using the Anthropic API
  for structured entity/relation extraction from unstructured text.
- **Observability (planned)**: structured logging (spdlog) replacing
  `std::cout`, Prometheus metrics endpoint, Grafana dashboard,
  OpenTelemetry tracing across service calls.
- **Containerization**: multi-stage Dockerfiles per service (build stage
  with full toolchain, slim runtime stage).
- **Orchestration**: Kubernetes manifests for a local `kind` cluster,
  later replaced by a Helm chart.
- **Infra-as-code (planned)**: Terraform for cluster/resource
  provisioning, so environment setup isn't a manual/undocumented step.
- **CI/CD**: GitHub Actions — build + test C++ on every push today;
  planned matrix build, Docker image build/push to GHCR, and Python
  lint/test once the extraction service exists.
- **Frontend (planned)**: React + TypeScript + D3.js dashboard
  visualizing the graph and surfacing detected contradictions.
- **Benchmarking / perf engineering**: in-repo C++ microbenchmark
  (`bench/contradiction_bench.cpp`) measuring min/p50/p99/max latency of
  each detector method at 100/1k/10k-edge graph sizes — already found a
  real O(n²)-per-predicate-group bottleneck (see Current status). Planned
  follow-up: k6/wrk load test of the HTTP API, and a benchmark vs. a naive
  SQLite baseline, published in the README with real numbers.
- **ML infra / vector search**: `SemanticIndex` (brute-force cosine
  similarity over `std::vector<float>` embeddings) plus `embed_text()`, a
  hashed-trigram bag-of-features embedding used as a placeholder until a
  real embedding model/API is wired in. Intended use: catch contradictions
  that don't share exact predicate/object strings but are semantically
  the same claim. Today it's an untested-in-production, self-hosted
  ANN-style index — not yet plugged into `ContradictionDetector`.
- **RL — contradiction ranking**: `EpsilonGreedyRanker`
  (`services/extraction/ranking/bandit.py`), a multi-armed bandit that
  learns, from user feedback (acted-on vs. dismissed), which category of
  contradiction is worth surfacing first, and re-ranks accordingly. Explore
  vs. exploit via epsilon-greedy; per-category value estimated by
  incremental sample averaging. Not yet wired to any real feedback source
  or persisted between runs — currently a standalone, tested-by-hand module.

Every "planned" item above stays marked as such until it's built and
tested — don't let the roadmap's ambition drift into overstating current
status.

## Design decisions already made (don't relitigate without discussion)

- **C++ for the storage/systems layer, Python for ML/NLP** — deliberate
  split chosen for resume value: demonstrates both systems programming
  and ML engineering, each in the language where it's most natural.
- **DTO pattern for the JSON API** — external API shape (`from`/`to`,
  string enum values) is deliberately decoupled from internal struct
  field names (`subject_id`/`object_id`, enum classes). Translation layer
  lives in `json_translation.hpp/cpp`, kept separate from `main.cpp`
  specifically so it's independently unit-testable.
- **Bi-temporal edges**: every Edge has `valid_at`, `recorded_at`, and
  optional `invalid_at`. Nothing is ever deleted on revision — an edge
  gets invalidated (invalid_at set), never removed. This is core to the
  whole design; don't "simplify" it away.
- **Contradiction detection lives in its own module**
  (`contradiction_detector.hpp/cpp`), not inside `GraphStore` — `GraphStore`
  only exposes raw edge access (`live_edges()`, `all_edges()`, etc.);
  detection logic is a separate, independently testable layer on top.
  Same separation-of-concerns reasoning as the DTO layer.
- **Semantic search is a separate, swappable module**
  (`semantic_index.hpp/cpp`): `SemanticIndex` only knows about
  `Embedding` (`std::vector<float>`) and IDs, never about `Node`/`Edge`
  directly — so the placeholder hashed-trigram `embed_text()` can be
  replaced by a real embedding-model call later without touching the
  index or search logic. Same reasoning as the DTO layer: keep the thing
  that will change (how you get an embedding) decoupled from the thing
  that won't (how you search a set of them).
- **Contradiction ranking is a pure function of feedback, not of the
  graph** — `EpsilonGreedyRanker` takes a flat list of `Contradiction`
  records (id, category, confidence) and returns a re-ordered list; it
  has no dependency on `GraphStore` or C++ types at all. Keeps the RL
  component testable and swappable independent of the storage engine.
- **Persistence is a write-ahead log, decoupled from `GraphStore`**
  (`persistence.hpp/cpp`): `WalWriter` appends one JSON-line op
  (`add_node`/`add_edge`/`invalidate_edge`) per mutation, and
  `load_graph_store_from_wal()` replays a log into a fresh `GraphStore` on
  startup. `GraphStore` itself stays a pure in-memory structure with no
  knowledge of durability — same layering principle as the DTO and
  contradiction-detection modules. Not yet wired into `main.cpp`.
- **Fail loud on bad input**: JSON parsing/translation throws on missing
  fields or invalid enum strings, caught at the HTTP layer and converted
  to a 400 with a clear error message. Don't silently default or coerce
  bad input.
- **Multi-stage Docker builds** — build stage compiles with full
  toolchain, runtime stage copies only the final binary. Keep this
  pattern for every service.
- **CMake + FetchContent for dependencies** (Catch2 fetched this way) —
  keeps the project self-contained; anyone cloning the repo gets the
  right tool versions automatically without manual installs beyond a
  compiler and CMake itself.
- **API keys / secrets**: when the extraction service (which will call
  the Anthropic API) is built, the API key goes through a Kubernetes
  Secret, never hardcoded or committed. A `k8s/secret.yaml.example`
  template with a placeholder gets committed; the real `k8s/secret.yaml`
  is gitignored.

## Current status (as of last session)

- Storage service: `GraphStore` + JSON API fully working locally, unit
  tested (Catch2), CI green on GitHub Actions.
- `ContradictionDetector`: direct-contradiction detection,
  value/behavior mismatch detection, and five-state drift classification
  (HELD / REFINED / CONTRADICTED / BOTH / SUPERSEDED), fully unit tested
  (`test_contradiction_detector.cpp`, 11 test cases, all passing).
- `WalWriter` / `load_graph_store_from_wal()`: write-ahead-log durability
  layer implemented and compiling, **but not yet wired into `main.cpp`** —
  the HTTP server still uses a plain in-memory `GraphStore` with no
  persistence on restart. Wiring it in (construct a `WalWriter` in
  `main()`, call `record_*` after every mutating endpoint, call
  `load_graph_store_from_wal()` at startup instead of a fresh
  `GraphStore`) is the next concrete step, not yet done.
- `SemanticIndex` / `embed_text()` / `cosine_similarity()`: implemented and
  unit tested (`test_semantic_index.cpp`). `embed_text()` is explicitly a
  placeholder (hashed-trigram bag, no real model) — do not describe this
  as "using embeddings from a model" until it's swapped for a real one.
  Not yet called from anywhere in the request path.
- `contradiction_bench.cpp`: microbenchmark added and run locally. **Real
  finding, not hypothetical**: `find_direct_contradictions()` and
  `find_value_behavior_mismatches()` are O(n²) *within* each
  (subject, predicate) group, and at 10,000 edges spread over 20
  predicates (~500 edges/group) that's ~380ms mean latency — up from
  ~4.7ms at 1,000 edges. `classify_drift()` stays sub-millisecond at all
  three sizes tested (100/1k/10k) since it only touches one group.
  **This is a genuine, not-yet-fixed perf bottleneck** — the honest
  before-benchmark-numbers, and the fix (bucket live edges by object_id
  within a group instead of enumerating all pairs) is Tier 2 item 8
  below. Don't claim this is fast until it's actually fixed and
  re-benchmarked.
- `EpsilonGreedyRanker` (`bandit.py`): implemented, smoke-tested by hand
  (not yet a pytest suite), not wired to any real feedback source, and
  learned value estimates are in-memory only (lost on process restart —
  no persistence for the bandit's learned state yet).
- Full test suite: 74 assertions across 26 C++ test cases, all green.
- Dockerfile for storage service: not started.
- README: not started.
- Nothing outside `services/storage` and the one new `ranking/bandit.py`
  file started yet.

## Roadmap (prioritized, in order)

**Tier 1 — finish the storage service:**
1. ~~Real JSON API~~ — DONE
2. ~~Unit tests + CI~~ — DONE
3. ~~Contradiction-detection core (direct + value/behavior + drift)~~ — DONE
4. ~~Catch2 tests for `ContradictionDetector`~~ — DONE
5. ~~WAL-based persistence module (`WalWriter` + replay)~~ — DONE
6. Wire persistence into `main.cpp` (write on every mutation, load on
   startup) — NOT STARTED
7. Structured logging (spdlog) replacing `std::cout` in main.cpp — NOT STARTED

**Tier 2 — prove it with numbers (quant-dev / perf-engineering angle):**
8. Fix the O(n²)-per-group bottleneck found by `contradiction_bench.cpp`:
   bucket live edges within a (subject, predicate) group by `object_id`
   first (single pass) so distinct-object contradictions are found in
   roughly O(n) instead of enumerating all pairs; re-run the benchmark
   and record the before/after numbers directly in this file and the
   README. This is the single most resume-relevant "found it, measured
   it, fixed it, proved it" story in the project — don't skip it for a
   flashier item.
9. Extend `contradiction_bench.cpp` (or add a sibling benchmark) to cover
   `SemanticIndex::most_similar()` at increasing index sizes, since
   brute-force cosine similarity is also O(n) per query and will need a
   real ANN structure (e.g. HNSW) once the index is large enough to matter.
10. Benchmark/comparison writeup vs. a naive SQLite baseline — real
    numbers (query latency, memory footprint, req/sec) for the README.
11. Expose contradiction/drift results over the HTTP API
    (`GET /contradictions`, `GET /drift/:subject_id/:predicate`).

**Tier 3 — distributed systems, ML infra, and RL integration:**
12. Extraction service: Python, calls the Anthropic API for entity/relation
    extraction, exposes its own HTTP API, containerized with the same
    multi-stage Docker pattern as storage.
13. Swap `embed_text()`'s hashed-trigram placeholder for real embeddings
    (an actual embedding model or API call) and wire `SemanticIndex` into
    `ContradictionDetector` so semantically-equivalent claims (not just
    exact predicate/object string matches) get flagged — this is what
    turns the vector index from a standalone module into an actual
    ML-infra feature of the product.
14. Wire `EpsilonGreedyRanker` into the storage API: an endpoint that
    returns contradictions pre-ranked by the bandit, plus a
    `POST /feedback` endpoint that calls `record_feedback()` so the
    ranker actually learns from real usage instead of hand-fed rewards;
    persist the bandit's learned value estimates (reuse the WAL pattern
    or a small JSON snapshot) so learning survives a restart.
15. gRPC + Protocol Buffers between extraction and storage (in addition
    to the public REST API).
16. Redis: caching layer for storage's hot read paths, plus an event
    stream (Redis Streams/NATS) for async extraction → storage ingestion.
17. Lightweight API gateway in front of both services: API-key auth,
    rate limiting.
18. `kind` cluster + Kubernetes manifests deploying storage + extraction
    + gateway as separate pods talking over k8s Services.
19. Helm chart packaging (replacing raw k8s YAML).
20. Terraform for cluster/resource provisioning.
21. Prometheus metrics endpoint + Grafana dashboard + OpenTelemetry tracing
    (should include the bandit's per-category value estimates and the
    detector's benchmarked latencies as tracked metrics, not just
    infra-level metrics).

**Tier 4 — presentation, do last:**
22. React + TypeScript + D3.js web UI visualizing the graph and
    surfacing detected contradictions, ranked by the bandit.
23. Full README rewrite: architecture diagram (Mermaid), badges, "why I
    built this," benchmark numbers (detector latency before/after the
    Tier 2 fix, semantic search vs. exact match) front and center.

## Working conventions for Claude Code sessions on this repo

- Work in small increments — one Tier item (or a clear sub-piece of one)
  per session, not the whole roadmap at once.
- Every C++ addition needs: the header, the implementation, and a Catch2
  test file, in that order, before moving to the next piece.
- No comments in code — see "Owner's background and working style" above.
- Run the full test suite (`cmake --build build && ./build/unit_tests`)
  before considering any change done.
- Commit after each working, tested unit — don't batch multiple
  unrelated changes into one commit. Don't push unless explicitly asked.
- If a design decision isn't covered above and isn't obvious, stop and
  ask rather than guessing — this file is the source of truth for
  established decisions, but it won't cover everything.
