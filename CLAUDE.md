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
        persistence.hpp          — WAL-based durability for GraphStore (DONE, wired into main.cpp)
        semantic_index.hpp       — embedding + cosine-similarity vector index (DONE, placeholder embeddings)
        logger.hpp               — dependency-free structured logger (DONE, see note below on spdlog)
        httplib.h                — vendored single-header HTTP library
        json.hpp                 — vendored nlohmann/json single header
      src/
        graph_store.cpp
        contradiction_detector.cpp
        json_translation.cpp
        persistence.cpp
        semantic_index.cpp
        logger.cpp
        main.cpp                 — HTTP server entrypoint (DONE): loads GraphStore
                                    from the WAL at startup, records every
                                    POST /nodes and POST /edges to the WAL, and
                                    logs structured startup/error events
      tests/
        test_graph_store.cpp      — Catch2 unit tests (DONE, passing)
        test_json_translation.cpp — Catch2 unit tests (DONE, passing)
        test_contradiction_detector.cpp — Catch2 unit tests (DONE, passing)
        test_semantic_index.cpp  — Catch2 unit tests (DONE, passing)
      bench/
        contradiction_bench.cpp  — latency/throughput microbenchmark for
                                    ContradictionDetector at increasing graph
                                    sizes (DONE, see Current status for findings)
        semantic_index_bench.cpp — latency microbenchmark for
                                    SemanticIndex::most_similar() at increasing
                                    index sizes (DONE)
        sqlite_baseline_bench.cpp — naive SQLite self-join baseline for direct
                                    contradiction detection, for comparison
                                    against ContradictionDetector (DONE, see
                                    Current status for the real numbers)
      CMakeLists.txt              — FetchContent for Catch2, links system
                                    libsqlite3 for the baseline bench (DONE)
      Dockerfile / .dockerignore  — multi-stage build (DONE, built and run
                                    locally with docker build/run — see
                                    Current status)
    extraction/                  — Python LLM-based extraction pipeline (IN PROGRESS)
      Calls the Anthropic API to turn raw journal/chat text into typed
      Node/Edge candidates, posts them to the storage service's REST API.
      Planned stack: FastAPI, Pydantic, httpx.
      ranking/
        bandit.py                — epsilon-greedy contextual bandit (RL) that
                                    ranks detected contradictions by learned
                                    category value + confidence (DONE)
        test_bandit.py            — unittest suite for the bandit (DONE, passing)
        persistence.py            — save_state()/load_state() for the bandit's
                                    learned value estimates, as a JSON file
                                    (DONE)
        test_persistence.py       — unittest suite, including a real
                                    save-restart-still-ranked-correctly
                                    round trip (DONE, passing)
        service.py                — stdlib-only HTTP service (no framework
                                    dependency) exposing POST /rank and
                                    POST /feedback over the bandit, now
                                    loading/saving state via persistence.py
                                    (DONE, manually verified across a real
                                    process restart — see Current status).
                                    Nothing else in the system calls it yet.
        Dockerfile                — containerizes the ranking service
                                    (python:3.12-slim, stdlib only) (DONE)
    gateway/                     — lightweight API gateway in front of storage:
      X-API-Key auth + per-key token-bucket rate limiting, proxies allowed
      requests through to the storage service (DONE)
      rate_limiter.py             — TokenBucket + RateLimiter (DONE, unit
                                    tested with injected fake clock values,
                                    not real sleeps — test_rate_limiter.py)
      gateway.py                  — stdlib-only HTTP proxy; `evaluate_request()`
                                    is a pure function (auth + rate-limit
                                    decision) kept separate from the HTTP
                                    handler specifically so it's unit
                                    testable without real sockets (DONE)
      test_gateway.py             — unit tests for `evaluate_request()` (DONE)
      Dockerfile                  — containerizes the gateway (DONE)
  k8s/                            — Kubernetes manifests for a local `kind`
                                    cluster
    storage-deployment.yaml       — Deployment for the storage service, with
                                    readiness/liveness probes against
                                    /health (DONE, YAML-syntax validated
                                    offline; NOT YET applied to a real
                                    cluster — no `kind`/cluster available in
                                    this session, see Current status)
    storage-service.yaml          — ClusterIP Service exposing it (DONE, same
                                    caveat as above)
  infra/
    helm/                         — Helm chart replacing raw manifests (PLANNED)
    terraform/                   — IaC for the `kind`/cloud cluster + any
      managed resources (Redis, object storage) (PLANNED)
  web/                            — React + TypeScript + D3.js graph
    visualization dashboard (PLANNED, Tier 4)
  .github/workflows/
    ci.yml                       — builds + runs C++ tests on every push (DONE, green)
  docker-compose.yml              — brings up storage + ranking + gateway
                                    together on one network (DONE, verified:
                                    `docker compose up`, curled the gateway,
                                    watched it proxy to storage over the
                                    compose network, `docker compose down`
                                    cleaned up — see Current status)
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
- **Observability**: structured key=value logging is DONE
  (`logger.hpp/cpp`) and wired into `main.cpp` — but it's a small
  dependency-free logger, not spdlog. spdlog was the original plan; it
  was skipped to avoid adding a new FetchContent dependency without a
  chance to verify network reliability in a given session. Swapping the
  backend for spdlog later is a drop-in change since callers only see
  `kg::log::info/warn/error`. Prometheus metrics endpoint, Grafana
  dashboard, and OpenTelemetry tracing remain PLANNED (Tier 3).
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
- **Auth/rate-limit decisions are pure functions, kept separate from the
  HTTP handler that calls them** — `gateway.py`'s `evaluate_request()`
  takes primitives in, returns a primitive tuple out, with no socket or
  `self` involved. Same reasoning as `EpsilonGreedyRanker`'s
  `rank()`/`export_state()`: the thing worth unit testing shouldn't
  require spinning up a real server to test. This is also what made it
  possible to catch the `TokenBucket` clock bug (see Current status)
  from a plain `unittest` run instead of a flaky integration test.

## Current status (as of last session)

- Storage service: `GraphStore` + JSON API fully working locally, unit
  tested (Catch2), CI green on GitHub Actions.
- `ContradictionDetector`: direct-contradiction detection,
  value/behavior mismatch detection, and five-state drift classification
  (HELD / REFINED / CONTRADICTED / BOTH / SUPERSEDED), fully unit tested
  (`test_contradiction_detector.cpp`, 11 test cases, all passing).
- `WalWriter` / `load_graph_store_from_wal()`: **wired into `main.cpp`
  and manually verified end-to-end** — `main()` now loads `GraphStore`
  from `storage.wal` at startup and records every `POST /nodes` and
  `POST /edges` to it. Verified by hand: posted a node, killed the
  server, restarted it, `GET /nodes/:id` still returned it. There is no
  HTTP endpoint that invalidates an edge yet, so `record_invalidate_edge`
  is implemented but has no caller.
- `logger.hpp/cpp`: dependency-free structured logger, wired into
  `main.cpp` in place of the old `std::cout` line — startup and
  request-rejection events now emit `time=... level=... msg="..." key="value"`
  lines. Verified by hand (see log output in this session). Not spdlog —
  see the Tech stack section for why.
- `SemanticIndex` / `embed_text()` / `cosine_similarity()`: implemented and
  unit tested (`test_semantic_index.cpp`). `embed_text()` is explicitly a
  placeholder (hashed-trigram bag, no real model) — do not describe this
  as "using embeddings from a model" until it's swapped for a real one.
  Not yet called from anywhere in the request path.
- **The O(n²)-per-group bottleneck is FIXED and re-benchmarked** —
  `find_direct_contradictions()` and `find_value_behavior_mismatches()`
  now bucket live edges by `object_id` within each (subject, predicate,
  edge_class) group (single pass) instead of enumerating all pairs.
  Real before/after numbers from `contradiction_bench.cpp` at 10,000
  edges / 20 predicates: `find_direct_contradictions()` mean latency went
  from **~381ms to ~8.0ms** (~47x), `find_value_behavior_mismatches()`
  from **~384ms to ~8.2ms** (~47x). All 74 existing assertions still pass
  unchanged — this was a pure complexity fix, not a behavior change, and
  the existing tests (which only ever exercised 2-edge groups) couldn't
  have caught the difference either way, so treat this as verified by
  the benchmark, not by the unit tests. `classify_drift()` was already
  fine and is untouched.
- `sqlite_baseline_bench.cpp`: added and run. A naive SQLite self-join
  over the same synthetic data (`SELECT ... FROM edges a JOIN edges b ON
  ...`) takes **~284ms mean at 10,000 edges** — i.e. the optimized
  in-memory `ContradictionDetector` (~8.0ms) is roughly **35x faster**
  than the naive SQL approach at the same scale. This is a real,
  reproducible number, not an estimate — rerun both benches to check it.
- `EpsilonGreedyRanker` (`bandit.py`): has a real `unittest` suite
  (`test_bandit.py`, 6 tests) covering explore/exploit ranking and
  incremental value updates, **plus `export_state()`/`load_state()`**
  for serializing learned per-category value estimates.
- `services/extraction/ranking/persistence.py` +
  `test_persistence.py`: JSON-file save/load for the bandit's state (3
  tests, including a round-trip through an actual restored ranker
  producing the same ranking as the original). Wired into `service.py`:
  loads `bandit_state.json` at startup, saves after every
  `POST /feedback`. **Verified with a real process restart**: fed one
  piece of feedback, killed the process, restarted it, and a fresh
  `POST /rank` call already reflected the learned preference — see
  transcript in this session. State file is gitignored
  (`bandit_state.json`).
- `services/extraction/ranking/service.py`: now containerized
  (`Dockerfile`, verified via `docker compose build`/`up`), but still
  nothing *calls* it as part of a real flow — the extraction pipeline
  that would produce contradictions to rank doesn't exist yet.
- `GET /contradictions` and `GET /drift/:subject_id/:predicate`: added
  to `main.cpp` and **verified by hand end-to-end**: posted a direct
  contradiction (same subject+predicate, different object) and a
  value/behavior mismatch, confirmed both show up correctly typed in
  `/contradictions`, confirmed `/drift/self/lives_in` returns
  `CONTRADICTED` and `/drift/self/prioritizes` returns `BOTH`, and
  confirmed an unknown subject/predicate 404s instead of 500ing.
- **API gateway (`services/gateway/`): built and verified two ways.**
  First, `evaluate_request()` (the auth + rate-limit decision, kept as a
  pure function separate from the HTTP handler) has 10 passing unit
  tests. Building those tests caught a real bug: `TokenBucket` seeded
  `last_refill` from the real wall clock (`time.monotonic()`) while
  tests fed it synthetic `now=0.0` timestamps, producing a huge bogus
  elapsed-time value on the first call and making every bucket start
  already drained — fixed by lazily setting `last_refill` from whatever
  `now` value is first observed, real or synthetic. Second, ran the
  gateway against a real `storage_server` process: no API key → 401,
  wrong key → 401, correct key → request actually proxied through to
  storage and back (`POST /nodes` then `GET /nodes/:id` via the gateway
  both worked).
- **`docker-compose.yml`: built and run for real.** `docker compose
  build` built all three images (storage, ranking, gateway);
  `docker compose up` started all three on one network; curled the
  gateway from the host with the `X-API-Key` header and it proxied to
  the `storage` container by its compose service name
  (`STORAGE_URL=http://storage:8080`), not localhost — i.e. real
  container-to-container networking, not just three processes sharing a
  host. `docker compose down` cleaned up. This is the most concrete
  "distributed system" demonstration in the project so far.
- `semantic_index_bench.cpp`: `SemanticIndex::most_similar()` stays in
  the low single-digit milliseconds up to 10,000 embeddings (brute-force
  cosine, O(n) per query) — no bottleneck found yet at these sizes.
  Worth re-checking once the index is actually populated from real data.
- **Dockerfile for storage service: written AND verified working.**
  `docker build` succeeds (multi-stage, Ubuntu 22.04 builder → slim
  runtime, only the `storage_server` binary copied into the final
  image), and the container was actually run: `/health`, `POST /nodes`,
  `GET /nodes/:id`, and `/stats` all worked against the containerized
  binary, and structured log lines appeared on `docker logs`. `.dockerignore`
  excludes the host's `build/` directory, which is required — without it
  the container would inherit a macOS-configured `CMakeCache.txt` and
  fail to reconfigure on Linux.
- `k8s/storage-deployment.yaml` and `k8s/storage-service.yaml`: written,
  and YAML-syntax/structure validated offline (no cluster available in
  this session — `kubectl` is installed but there's no `kind` cluster or
  any other cluster to point it at, and `kubectl apply --dry-run=client`
  still tries to contact a server for API discovery and fails without
  one). **Not yet actually applied to a running cluster — don't claim
  they're deploy-tested until that happens.**
- Full test suite: 74 C++ assertions across 26 test cases + 19 Python
  unittest cases (9 ranking + 10 gateway), all green.
- README: not started.
- `services/extraction` now has real content (`ranking/`), but the
  extraction pipeline itself (calling the Anthropic API to produce
  Node/Edge candidates) is still not started.

## Roadmap (prioritized, in order)

**Tier 1 — finish the storage service:**
1. ~~Real JSON API~~ — DONE
2. ~~Unit tests + CI~~ — DONE
3. ~~Contradiction-detection core (direct + value/behavior + drift)~~ — DONE
4. ~~Catch2 tests for `ContradictionDetector`~~ — DONE
5. ~~WAL-based persistence module (`WalWriter` + replay)~~ — DONE
6. ~~Wire persistence into `main.cpp`~~ — DONE, verified with a real
   restart test (see Current status)
7. ~~Structured logging wired into `main.cpp`~~ — DONE, dependency-free
   logger rather than spdlog (see Tech stack note on why)

**Tier 2 — prove it with numbers (quant-dev / perf-engineering angle):**
8. ~~Fix the O(n²)-per-group bottleneck found by `contradiction_bench.cpp`~~
   — DONE, ~47x faster at 10k edges. Real before/after numbers in
   Current status — this is the project's clearest "found it, measured
   it, fixed it, proved it" story; keep it front and center in the README.
9. ~~Extend the benchmark suite to cover `SemanticIndex::most_similar()`~~
   — DONE (`semantic_index_bench.cpp`); no bottleneck found up to 10k
   embeddings, re-check once real data populates the index.
10. ~~Benchmark vs. a naive SQLite baseline~~ — DONE
    (`sqlite_baseline_bench.cpp`); optimized detector is ~35x faster than
    a naive self-join at 10k edges. Still missing: memory-footprint and
    req/sec numbers (this only measured query latency) — worth adding if
    the README wants a fuller comparison table.
11. ~~Expose contradiction/drift results over the HTTP API~~ — DONE
    (`GET /contradictions`, `GET /drift/:subject_id/:predicate`),
    verified end-to-end with real posted data (see Current status).

**Tier 3 — distributed systems, ML infra, and RL integration:**
12. Extraction service: Python, calls the Anthropic API for entity/relation
    extraction, exposes its own HTTP API, containerized with the same
    multi-stage Docker pattern as storage. NOT STARTED (needs an API key
    this session doesn't have — do this with Surya present to supply one).
13. Swap `embed_text()`'s hashed-trigram placeholder for real embeddings
    (an actual embedding model or API call) and wire `SemanticIndex` into
    `ContradictionDetector` so semantically-equivalent claims (not just
    exact predicate/object string matches) get flagged — this is what
    turns the vector index from a standalone module into an actual
    ML-infra feature of the product. NOT STARTED (same API-key blocker).
14. ~~Persist the bandit's learned value estimates~~ — DONE
    (`persistence.py`, wired into `service.py`, verified across a real
    restart). ~~Add a Docker entry so it isn't a manually-started
    script~~ — DONE (`services/extraction/ranking/Dockerfile`, part of
    `docker-compose.yml`). **Still open**: actually call `service.py`'s
    `POST /rank`/`POST /feedback` from somewhere real (the future
    extraction service or a UI) instead of curling it by hand — there's
    still no real caller.
15. gRPC + Protocol Buffers between extraction and storage (in addition
    to the public REST API).
16. Redis: caching layer for storage's hot read paths, plus an event
    stream (Redis Streams/NATS) for async extraction → storage ingestion.
17. ~~Lightweight API gateway in front of storage: API-key auth, rate
    limiting~~ — DONE (`services/gateway/`), verified against a real
    `storage_server` process AND through `docker-compose.yml` with real
    container-to-container networking. Not yet in front of the
    extraction/ranking services too, since there's no real traffic to
    those yet — revisit once item 12 exists.
18. `kind` cluster set up locally + `kubectl apply` of
    `k8s/storage-deployment.yaml` and `k8s/storage-service.yaml` (written
    and YAML-validated already, see Current status) against a real
    cluster; then add extraction + gateway manifests once those services
    exist. **Partially done**: the storage manifests exist but are
    unverified against an actual cluster — installing `kind` was out of
    scope for this session. `docker-compose.yml` now covers local
    multi-service orchestration in the meantime, but that's not the same
    thing as a k8s deployment — don't conflate the two on a resume.
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
- Every Python addition needs a matching `unittest`-based test file
  (stdlib only, no pytest) in the same directory, run with
  `python3 -m unittest <file>.py`.
- Run the full test suite (`cmake --build build && ./build/unit_tests`)
  before considering any change done, plus any Python test files touched.
- Commit after each working, tested unit — don't batch multiple
  unrelated changes into one commit. Don't push unless explicitly asked.
- If a design decision isn't covered above and isn't obvious, stop and
  ask rather than guessing — this file is the source of truth for
  established decisions, but it won't cover everything.
