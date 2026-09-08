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
                                      mismatch, drift-state, and (new)
                                      semantic value/behavior mismatch
                                      detectors (DONE — see Current status
                                      for exactly what "semantic" does and
                                      doesn't mean here)
        json_translation.hpp     — DTO layer, JSON <-> C++ types (DONE)
        persistence.hpp          — WAL-based durability for GraphStore (DONE, wired into main.cpp)
        semantic_index.hpp       — embedding + cosine-similarity vector index (DONE, placeholder embeddings)
        logger.hpp               — dependency-free structured logger (DONE, see note below on spdlog)
        metrics.hpp              — Prometheus-exposition-format request counters
                                    (DONE, see Current status for a known
                                    cardinality caveat)
        redis_client.hpp         — minimal RESP-protocol client over a raw POSIX
                                    socket (GET/SET/DEL/PING only, no hiredis
                                    dependency) (DONE)
        node_cache.hpp           — domain-specific cache-aside wrapper: Node <->
                                    JSON <-> RedisClient, kept separate from the
                                    raw protocol client (DONE)
        grpc_server.hpp          — GrpcStorageService: AddNode/AddEdge/GetNode
                                    over gRPC, sharing the same GraphStore, WAL,
                                    and NodeCache as the REST handlers (DONE,
                                    see docs/GRPC.md and Current status)
        httplib.h                — vendored single-header HTTP library
        json.hpp                 — vendored nlohmann/json single header
      src/
        graph_store.cpp
        contradiction_detector.cpp
        json_translation.cpp
        persistence.cpp
        semantic_index.cpp
        logger.cpp
        metrics.cpp
        redis_client.cpp
        node_cache.cpp
        grpc_server.cpp
        main.cpp                 — HTTP server entrypoint (DONE): loads GraphStore
                                    from the WAL at startup, records every
                                    POST /nodes and POST /edges to the WAL, logs
                                    structured startup/error events, caches nodes
                                    in Redis (cache-aside on GET, write-through on
                                    POST), exposes GET /metrics, exposes
                                    GET /nodes and GET /edges (list-all, added
                                    for the web frontend — see web/), sends
                                    permissive CORS headers (Access-Control-
                                    Allow-Origin: *) plus a catch-all OPTIONS
                                    preflight handler so a browser on a
                                    different origin/port can call it directly,
                                    and (when built with gRPC support) starts
                                    GrpcStorageService on a detached background
                                    thread on port 50051 alongside the HTTP
                                    server. Also exposes
                                    GET /contradictions/semantic?threshold=0.5
                                    (find_semantic_value_behavior_mismatches)
      tests/
        test_graph_store.cpp      — Catch2 unit tests (DONE, passing)
        test_json_translation.cpp — Catch2 unit tests (DONE, passing)
        test_contradiction_detector.cpp — Catch2 unit tests (DONE, passing)
        test_semantic_contradictions.cpp — 5 Catch2 tests for
                                    find_semantic_value_behavior_mismatches():
                                    flags near-identical text, ignores
                                    unrelated text, skips exact
                                    subject+predicate+object duplicates,
                                    respects the threshold, ignores subjects
                                    with no BehaviorEvidence edges at all
                                    (DONE, passing — verified over real HTTP
                                    too, see Current status)
        test_semantic_index.cpp  — Catch2 unit tests (DONE, passing)
        test_metrics.cpp         — Catch2 unit tests (DONE, passing)
        test_redis_client.cpp    — Catch2 unit tests: one pure failure-mode test
                                    that always runs (connect to an unreachable
                                    port, confirm graceful `false`/`nullopt`, no
                                    crash) plus real integration tests that SKIP
                                    (not fail) when no local redis-server is
                                    reachable, and actually assert against a
                                    real one when it is (DONE — see Current
                                    status for both cases actually being run)
        test_grpc_server.cpp     — 4 Catch2 tests, each starting a real
                                    in-process `grpc::Server` bound to an
                                    ephemeral port and a real client channel to
                                    it — not mocked. Covers AddNode+GetNode
                                    round-trip, GetNode on a missing id,
                                    AddNode rejecting an invalid type
                                    (asserts the actual gRPC status code), and
                                    AddEdge (DONE, only built/run when
                                    protobuf+gRPC are found — see below)
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
                                    libsqlite3 for the baseline bench, and
                                    conditionally builds the gRPC service:
                                    tries `find_package(... CONFIG)` first
                                    (Homebrew on macOS), falls back to
                                    pkg-config (Debian/Ubuntu, whose
                                    `libgrpc++-dev` has no CMake config
                                    package), skips gRPC entirely if neither
                                    is found — REST API unaffected either way
                                    (DONE)
      Dockerfile / .dockerignore  — multi-stage build, now built from the
                                    REPO ROOT as context (not services/storage/)
                                    so it can see ../../proto/, with apt
                                    packages for protobuf/gRPC via the
                                    pkg-config path above (DONE, built and run
                                    locally with docker build/run, both REST
                                    and gRPC verified inside the container —
                                    see Current status)
    extraction/                  — Python LLM-based extraction pipeline (DONE
                                    except the actual Anthropic call — see
                                    Current status)
      schemas.py                   — Pydantic models mirroring the C++ side's
                                    NodeType/EdgeClass exactly; NodeCandidate/
                                    EdgeCandidate use `from`/`to` aliases to
                                    match storage's JSON shape (DONE)
      extraction_client.py         — `ExtractionClient` ABC +
                                    `AnthropicExtractionClient` (raw `httpx`
                                    call to the Messages API, no `anthropic`
                                    SDK dependency). **Written but NOT run —
                                    no API key available this session, see
                                    Current status.**
      mock_extraction_client.py    — `MockExtractionClient`: real, working,
                                    regex-based heuristic extractor (e.g. "I
                                    value X" -> StatedValue edge, "I worked on
                                    X" -> BehaviorEvidence edge). This is the
                                    default backend — the service actually
                                    works out of the box with zero API key
                                    (DONE, unit tested)
      test_mock_extraction_client.py — unittest suite, 6 tests (DONE, passing)
      app.py                       — FastAPI app: `POST /extract` (just
                                    returns candidates) and
                                    `POST /extract-and-store` (extracts, then
                                    POSTs each node/edge to the storage
                                    service's real REST API) (DONE, verified
                                    end-to-end — see Current status). Also now
                                    `GET /contradictions/ranked` (fetches
                                    storage's `/contradictions`, ranks via
                                    `ranking_client`) and `POST /feedback`
                                    (forwards to ranking) — the actual closed
                                    detection-to-ranking loop, see
                                    docs/CONTRADICTION_RANKING.md
      test_app.py                  — FastAPI `TestClient` unittest suite,
                                    exercises the mock backend, no network
                                    calls (DONE, passing)
      cortexkernel_pb2.py,
      cortexkernel_pb2_grpc.py      — generated from proto/cortexkernel.proto
                                    (DONE — committed, since Python has no
                                    build-time codegen story the way CMake
                                    does; regenerate after changing the .proto,
                                    see docs/GRPC.md)
      grpc_client.py                — StorageGrpcClient, hand-written wrapper
                                    around the generated stubs. Verified against
                                    a real running storage gRPC server (DONE).
                                    Still not called by app.py's actual
                                    extraction flow — available, not integrated.
      test_grpc_client.py           — 4 unittest cases against a real
                                    in-process Python gRPC server (a small
                                    `FakeServicer`, not the C++ binary) — tests
                                    the client's request/response handling,
                                    including the `from`/`to` reserved-keyword
                                    field workaround, without needing the C++
                                    build at all (DONE, passing — this closed
                                    a real gap: grpc_client.py had zero test
                                    coverage before this round)
      ranking_client.py              — contradiction_to_rank_item() (pure,
                                    tested), rank_contradictions() (POSTs to
                                    ranking's /rank, re-orders the original
                                    contradiction objects by the returned
                                    ranking), submit_feedback() (POSTs to
                                    ranking's /feedback) (DONE — see
                                    docs/CONTRADICTION_RANKING.md for the
                                    full verified end-to-end loop)
      test_ranking_client.py         — unittest for the pure
                                    contradiction_to_rank_item() function and
                                    the empty-input case (DONE, passing)
      test_app_ranking.py            — unittest for app.py's
                                    GET /contradictions/ranked and
                                    POST /feedback routes, with
                                    rank_contradictions/submit_feedback/httpx.Client
                                    mocked out (no real network) (DONE, passing
                                    — first attempt at the storage-call mock
                                    had a real self-reference bug: patching
                                    `app.httpx.Client` also patched the
                                    `httpx.Client` call *inside* the
                                    replacement used to build the mock,
                                    since `app.httpx` is the same module
                                    object as the top-level `httpx` import;
                                    fixed with a plain `MagicMock` instead of
                                    a real `httpx.Client`+`MockTransport`)
      requirements.txt, Dockerfile — fastapi/uvicorn/httpx/pydantic/grpcio,
                                    built and run as a real container (DONE).
                                    `protobuf` is pinned to `>=7,<8` — see
                                    docs/GRPC.md for the real version-mismatch
                                    bug that pin exists to avoid.
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
  k8s/                            — Kubernetes manifests for all 5 services,
                                    ALL DONE and ALL actually deployed and
                                    verified against a real local `kind`
                                    cluster this session — not just written
                                    (see Current status for the full
                                    verification transcript)
    storage-deployment.yaml       — Deployment for storage, readiness/
                                    liveness probes against /health, now
                                    also sets REDIS_HOST/REDIS_PORT env vars
                                    pointing at the redis Service by name
    storage-service.yaml          — ClusterIP Service exposing it
    redis-deployment.yaml / redis-service.yaml — `redis:7-alpine`, ClusterIP,
                                    TCP-socket readiness probe (Redis has no
                                    HTTP health endpoint)
    ranking-deployment.yaml / ranking-service.yaml — ClusterIP, TCP-socket
                                    readiness probe (`service.py` has no
                                    `/health` route)
    extraction-deployment.yaml / extraction-service.yaml — ClusterIP, HTTP
                                    readiness probe against `/health`
                                    (FastAPI has one), `STORAGE_URL` pointed
                                    at the storage Service by name
    gateway-deployment.yaml / gateway-service.yaml — **NodePort**, the one
                                    deliberately public-facing entry point;
                                    everything else is ClusterIP
                                    (internal-only) — a real, intentional
                                    architecture decision, not an oversight
  helm/cortexkernel/              — Helm chart for the storage service (DONE)
    Chart.yaml, values.yaml       — chart metadata + configurable image/
                                    replica/Redis-host values
    templates/storage-deployment.yaml,
    templates/storage-service.yaml — templatized versions of the raw
                                    k8s/ manifests above, parameterized by
                                    `.Release.Name` and `.Values.*` (DONE,
                                    `helm lint` clean, `helm template`
                                    verified to render correctly — see
                                    Current status. Still NOT applied to a
                                    real cluster, same as the raw manifests)
  observability/                  — Prometheus + Grafana config (DONE, see
                                    Current status for a full working
                                    verification via docker-compose)
    prometheus.yml                 — scrape config pointed at storage:8080/metrics
    grafana/provisioning/datasources/prometheus.yml — auto-registers the
                                    Prometheus datasource on Grafana startup
    grafana/provisioning/dashboards/dashboards.yml  — tells Grafana where
                                    to load dashboard JSON from
    grafana/dashboards/cortexkernel.json — a real dashboard (2 panels:
                                    requests/sec by route, avg latency by
                                    route) built directly against the
                                    actual `cortexkernel_http_requests_total`
                                    / `..._duration_ms_sum` metric names
  infra/
    terraform/                   — IaC for the `kind`/cloud cluster + any
      managed resources (object storage) (PLANNED — blocked this session:
      Homebrew's terraform formula was pulled from homebrew-core, and the
      HashiCorp tap's bottle needs newer Xcode Command Line Tools than are
      installed here; installing those requires a system software update
      this session isn't going to push through on its own initiative)
  proto/
    cortexkernel.proto            — shared gRPC/protobuf schema for storage's
                                    AddNode/AddEdge/GetNode RPCs; the C++
                                    stubs are generated at build time (CMake),
                                    the Python stubs are generated once and
                                    committed (see docs/GRPC.md) (DONE)
  .dockerignore                   — repo-root dockerignore (needed once the
                                    storage Dockerfile started building from
                                    the repo root instead of services/storage/,
                                    so unrelated directories like web/node_modules
                                    or .git don't bloat the build context) (DONE)
  web/                            — React + TypeScript + D3.js graph
                                    visualization dashboard (DONE — see
                                    Current status for how it was verified)
    package.json, tsconfig.json, vite.config.ts — Vite + React + TS, D3 for
                                    the graph rendering. `npm run build`
                                    (tsc -b && vite build) actually run and
                                    passes, `npm run dev` actually started
                                    and served real modules.
    index.html, src/main.tsx      — app entrypoint
    src/api.ts                    — typed fetch wrappers for GET /nodes,
                                    GET /edges, GET /contradictions
                                    (VITE_API_URL, defaults to
                                    http://localhost:8080), plus
                                    fetchRankedContradictions() and
                                    submitFeedback() against the extraction
                                    service (VITE_EXTRACTION_API_URL,
                                    defaults to http://localhost:8082)
    src/GraphView.tsx             — D3 force-directed graph: draggable
                                    nodes, nodes involved in a detected
                                    contradiction rendered in red
    src/FeedbackButtons.tsx        — "Useful"/"Dismiss" buttons, calls
                                    submitFeedback() then triggers a
                                    re-fetch of the ranked list (DONE)
    src/ContradictionsPanel.tsx   — list of detected contradictions,
                                    ranked order, each with FeedbackButtons
                                    (DONE — now driven by
                                    fetchRankedContradictions(), not the
                                    raw unranked storage endpoint)
    src/App.tsx                   — fetches nodes/edges (storage, direct)
                                    and ranked contradictions (extraction)
                                    as two independent effects — so a down
                                    extraction service degrades the
                                    contradictions list, not the whole
                                    graph view — computes the
                                    contradicted-node set, wires feedback
                                    clicks to a re-fetch
  .github/workflows/
    ci.yml                       — builds + runs C++ tests on every push,
                                    now with a `redis` service container so
                                    the RedisClient integration tests
                                    actually assert instead of skipping in
                                    CI (DONE, green)
  docker-compose.yml              — brings up all 7 services (redis,
                                    storage, ranking, extraction, gateway,
                                    prometheus, grafana) together on one
                                    network (DONE, fully verified — see
                                    Current status: gateway proxying with
                                    real cache hits, Prometheus actually
                                    scraping storage's live /metrics and
                                    reporting the target `up`, Grafana
                                    auto-provisioning both the datasource
                                    and the dashboard with zero manual
                                    clicking, and the extraction service
                                    actually turning journal text into
                                    real nodes/edges inside storage over
                                    the compose network — plus, this round,
                                    end-to-end contradiction ranking:
                                    extraction now gets `RANKING_URL`
                                    pointed at the `ranking` service by
                                    name, and `scripts/demo.sh` proves the
                                    whole loop works through real compose
                                    networking, not just individually
                                    on native processes). storage's `build:`
                                    stanza now uses `context: .` (repo root)
                                    + an explicit `dockerfile:` path, not
                                    `./services/storage`, so its Dockerfile
                                    can see `proto/`; also now publishes
                                    port 50051 (gRPC) alongside 8080.
  README.md                       — full rewrite (DONE): badges, Mermaid
                                    architecture diagram, quickstart,
                                    "actually been run, not just written"
                                    section with real verified claims,
                                    tech stack table, links to docs/
  CHANGELOG.md                    — narrative build history grouped by
                                    milestone, derived from real `git log`
                                    (DONE)
  LICENSE                         — MIT (DONE)
  docs/
    ARCHITECTURE.md                — services, data model, design
                                    decisions, a Mermaid diagram (DONE)
    CONTRADICTION_DETECTION.md     — deep dive on the one novel idea:
                                    the three detectors, the five drift
                                    states, and the real O(n²) perf story
                                    (DONE)
    BENCHMARKS.md                  — the real numbers from Current status
                                    below, including the honest small-scale
                                    regression the fix introduces, not
                                    just the flattering 10k-edge number
                                    (DONE)
    API.md                         — every HTTP endpoint across all 4
                                    services, with real curl examples (DONE)
    DEPLOYMENT.md                  — docker-compose, bare-metal build,
                                    kind/Kubernetes, Helm, the frontend dev
                                    server — including the mbot port-8080
                                    gotcha as a documented caveat (DONE)
    GRPC.md                        — the second protocol into storage: what
                                    it does, how the C++/Python build paths
                                    both find protobuf/gRPC, and the real
                                    protobuf gencode/runtime version bug
                                    found while wiring up the Python side
                                    (DONE)
    CONTRADICTION_RANKING.md       — the closed detection-to-ranking loop
                                    (storage -> extraction -> ranking ->
                                    feedback -> re-ranking), verified with
                                    three live processes and curl, plus what
                                    the new semantic detector does and
                                    honestly doesn't do (DONE)
  scripts/
    demo.sh                        — one-command demo: docker compose up,
                                    extract a journal entry, seed a direct
                                    contradiction and a value/behavior one,
                                    show the ranked list, submit feedback,
                                    show it re-ranked. Actually run twice
                                    while building it — first run only
                                    produced one contradiction category
                                    (the mock extractor's "values"/"did"
                                    predicates differ, so it doesn't hit the
                                    exact-match detector), so the reorder
                                    wasn't visible; fixed by also seeding a
                                    same-predicate value/behavior pair
                                    directly. Second run's feedback
                                    happened to favor the category that was
                                    already ranked first by tie-breaking, so
                                    still no visible change; fixed by
                                    favoring the other one. Third run showed
                                    the reorder for real (DONE)
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
- **Redis is a cache-aside optimization, never a dependency the service
  needs to function** — `RedisClient` fails soft everywhere (returns
  `false`/`nullopt` instead of throwing) if it can't connect, and
  `main.cpp` still serves correct data straight from `GraphStore` when
  Redis is down (verified by hand — see Current status). Never make a
  future change that causes storage to hard-fail when Redis is
  unavailable; that would invert the whole point of it being a cache.
- **No hiredis dependency for `RedisClient`** — same reasoning as
  skipping spdlog: it's a small, well-specified text protocol (RESP), and
  a raw-socket implementation keeps the project's dependency surface
  small and every line of the client auditable/testable without pulling
  in a new library. Revisit only if the client needs to grow real
  connection pooling or pipelining.
- **`NodeCache` is a separate layer from `RedisClient`**, same reasoning
  as `SemanticIndex`/`embed_text()`: the raw protocol client knows
  nothing about `Node` or JSON; the domain-specific cache-key scheme
  (`cortexkernel:node:<id>`) and Node<->JSON translation live in
  `node_cache.hpp/cpp` instead.
- **Metrics record raw request paths today (e.g. `/nodes/n1`), not
  templated ones (e.g. `/nodes/:id`)** — this is a known, accepted
  cardinality footgun for a real Prometheus deployment with many
  distinct node IDs (each ID gets its own timeseries forever). Flagged
  here on purpose rather than silently shipped; fixing it means teaching
  the metrics/logging hook the matched route template, not just the
  resolved path — worth doing before this ever points at a Prometheus
  server with retention that matters.
- **Only the gateway is `NodePort`/publicly reachable in Kubernetes —
  storage, redis, ranking, and extraction are all `ClusterIP`
  (internal-only)** — the gateway is the intended single entry point
  (auth + rate limiting live there), so nothing else should be reachable
  from outside the cluster. Don't flip another service to `NodePort` as
  a debugging shortcut without reverting it.
- **A service's one-time startup log (e.g. storage's
  `redis_reachable="..."`) is a snapshot, not a live status** — in
  Kubernetes, pod start order isn't guaranteed, so storage can start and
  ping Redis before Redis's pod is actually accepting connections,
  logging `redis_reachable="false"` even though caching works correctly
  moments later once Redis comes up (verified — see Current status).
  Don't treat that one log line as authoritative; if this needs to be
  trustworthy later, it should retry/refresh instead of checking once.
- **gRPC runs inside the same process as the REST server, sharing the
  same `GraphStore`/`WalWriter`/`NodeCache`, not as a separate service
  with its own store** — `main.cpp` spawns `GrpcStorageService` on a
  detached background thread before `svr.listen()` blocks the main
  thread. This was a deliberate choice over a standalone
  `grpc_main.cpp` executable specifically so the two protocols are
  genuinely two doors into the same building, not two independent
  siloed graphs that happen to share a name. Verified: a node added via
  gRPC is immediately visible via REST, and lands in the same WAL file.
- **CMake detects protobuf/gRPC two ways on purpose**: `find_package(...
  CONFIG)` first (what Homebrew's builds provide on macOS), falling back
  to `pkg-config` (what Debian/Ubuntu's `libgrpc++-dev` provides instead
  — it ships no CMake config package at all, a genuine packaging gap).
  If neither path finds them, the gRPC service is skipped entirely and
  `storage_server` still builds REST-only — this is why CI (no
  protobuf/gRPC installed) stays green without needing those packages.
  Don't "simplify" this to a single `find_package(... CONFIG REQUIRED)`
  — that breaks the Ubuntu/Docker build path, which was verified to
  need the pkg-config fallback for real (see Current status).
- **Generated Python protobuf stubs are committed, not regenerated at
  build/install time** — unlike the C++ side (CMake generates them into
  the build directory), Python has no equivalent build-time codegen
  convention most people reach for, so `cortexkernel_pb2.py` and
  `cortexkernel_pb2_grpc.py` are checked into `services/extraction/`
  directly. Whoever regenerates them must also update
  `requirements.txt`'s `protobuf` pin to match the `protobuf`/
  `grpcio-tools` version used for codegen — see the version-mismatch bug
  in Current status and `docs/GRPC.md` for exactly why that pin matters
  and isn't just conservative-for-its-own-sake.
- **"Semantic" contradiction detection means text-similarity on the
  current placeholder embeddings, not real semantic understanding —
  say so every time this comes up, don't let the word "semantic" imply
  more than it does.** `find_semantic_value_behavior_mismatches()`
  embeds `predicate + " " + object_name` with the existing hashed-trigram
  `embed_text()` and flags cosine-similarity above a threshold. It
  reliably catches near-identical *text* (two edges whose object nodes
  happen to have the same or very similar name); it will NOT catch
  genuinely synonymous but differently-worded claims ("my health" vs.
  "staying healthy" share almost no character trigrams). This is
  deliberate, minimal, honest infrastructure for a real embedding model
  to slot into later (item 13) — don't describe it in any doc as "AI
  understands your contradictions" or similar. `test_semantic_contradictions.cpp`
  is written to test exactly this (text similarity), not semantics.
- **The frontend fetches nodes/edges and ranked contradictions as two
  independent effects in `App.tsx`, not one combined `Promise.all`** —
  ranked contradictions come from the extraction service, not storage
  directly, so if extraction is down, the graph should still render;
  only the contradictions panel should degrade. Don't merge these back
  into one fetch for "simplicity" — that was the original (wrong)
  design and it was deliberately split apart this round.

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
- **`GET /metrics` (Prometheus exposition format) and Redis caching:
  built and verified against a real, locally-installed Redis
  (`brew install redis`), not mocked.** `RedisClient` is a from-scratch
  RESP-protocol client over a raw POSIX socket (no hiredis) —
  `redis_client.cpp`. `NodeCache` wraps it with Node<->JSON translation
  and a `cortexkernel:node:<id>` key scheme. Wired into `main.cpp`:
  `POST /nodes` writes through to the cache, `GET /nodes/:id` is
  cache-aside and sets an `X-Cache: HIT`/`MISS` header. Verified by hand:
  POST then GET showed `X-Cache: HIT`; manually `DEL`-ing the key via
  `redis-cli` then GETing showed `MISS` then `HIT` on the next call.
  **Also verified graceful degradation**: killed the local redis-server
  entirely, restarted `storage_server`, confirmed POST/GET both still
  work correctly (falls back to `GraphStore`, `X-Cache: MISS` always) —
  the log line even reports `redis_reachable="false"` at startup for
  observability. Then verified the same thing the other way: brought up
  the full `docker-compose.yml` stack (now including a `redis` service)
  and confirmed `redis_reachable="true"` and caching actually worked
  over real container networking, not localhost.
- `test_redis_client.cpp` has one pure test that always runs (unreachable
  port → graceful `false`/`nullopt`, never a crash) plus three
  integration tests that `SKIP` (Catch2's `SKIP()`, not a failure) when
  no local redis-server is reachable. **Verified both branches**: ran the
  suite with no redis-server running (3 skipped, rest passed), then
  started one and reran (90/90 assertions actually executed, nothing
  skipped). `ci.yml` now runs a `redis` GitHub Actions service container
  specifically so these assert for real in CI instead of quietly skipping
  forever.
- `test_metrics.cpp`: 3 passing tests covering counting, per-route/status
  separation, and the empty-state case.
- Full test suite: 90 C++ assertions across 33 test cases (up from 74/26
  — added `test_metrics.cpp` and `test_redis_client.cpp`) + 19 Python
  unittest cases (9 ranking + 10 gateway), all green.
- **Helm chart (`helm/cortexkernel/`): written and verified with the real
  `helm` binary** (`brew install helm`), not just hand-checked YAML —
  `helm lint` passed clean, and `helm template my-release helm/cortexkernel`
  rendered the Deployment and Service with `.Release.Name` and
  `.Values.*` substituted correctly (image, replica count, Redis
  host/port env vars). **Still not applied to a real cluster** — same
  `kind`-not-installed limitation as the raw `k8s/` manifests, this only
  proves the chart is well-formed and renders correctly.
- **Prometheus + Grafana (`observability/`): built and fully verified
  end-to-end via `docker-compose.yml`, not just configured on paper.**
  Brought up the whole stack (`redis`, `storage`, `ranking`, `gateway`,
  `prometheus`, `grafana`), generated real traffic through the gateway,
  then confirmed: `curl localhost:9090/api/v1/targets` showed the
  `cortexkernel-storage` scrape target as `up`; `curl
  localhost:9090/api/v1/query?query=cortexkernel_http_requests_total`
  returned real scraped data, not zero results; `curl
  localhost:3000/api/datasources` showed the Prometheus datasource
  auto-provisioned; `curl
  localhost:3000/api/dashboards/uid/cortexkernel-storage` showed the
  dashboard auto-loaded with both panels, titled correctly — all without
  a single manual click in the Grafana UI. This is the most complete,
  actually-working piece of "observability" in the project so far, not
  aspirational config.
- Attempted Terraform for local docker orchestration (parity with
  `docker-compose.yml` but as IaC) and it's currently blocked: Homebrew
  removed `terraform` from homebrew-core over licensing, and the
  HashiCorp tap's bottle needs a newer Xcode Command Line Tools version
  than is installed on this machine — fixing that means a system
  software update, which this session isn't going to push through
  unprompted. Flagging so a future session doesn't waste time
  rediscovering this; ask Surya to update CLT first, or just skip
  Terraform and keep `docker-compose.yml` as the local-orchestration
  story (they overlap in purpose for local dev).
- **Extraction service: built and verified end-to-end, with one honest
  gap.** `MockExtractionClient` (regex-based, zero dependencies beyond
  the stdlib `re`) is the default backend and is genuinely working code —
  6 passing unit tests, plus a real pipeline test: posted
  `"I value my health. I worked on the project all night."` to
  `POST /extract-and-store`, and it correctly produced a `StatedValue`
  edge (`self --values--> my-health`) and a `BehaviorEvidence` edge
  (`self --did--> the-project-all-night`), which actually landed in a
  real running `storage_server` (`/stats` went from empty to
  `node_count=3, edge_count=2`). Repeated the same test over the full
  `docker-compose` network with the `extraction` service added — same
  result, real container-to-container traffic. **The honest gap**:
  `AnthropicExtractionClient` (the real LLM-based path) is written —
  a real `httpx` call to the Messages API with a structured-JSON-output
  prompt matching the exact `NodeCandidate`/`EdgeCandidate` schema — but
  has never actually been run, because there's no `ANTHROPIC_API_KEY`
  available in this session. Don't claim the LLM extraction path works
  until someone runs it with a real key and confirms the response
  actually parses as valid JSON matching the schema (LLM output not
  perfectly following a format is the realistic failure mode to expect).
- `helm` and `kind` were installed via Homebrew this session
  (`brew install helm kind`) — both are now available for future
  sessions on this machine.
- **`kind` cluster: actually created and deployed to, not just planned.**
  `kind create cluster --name cortexkernel`, built all 4 custom images
  (`docker build`), `kind load docker-image` to get them onto the
  cluster's node (kind can't see the host's local image cache
  otherwise), `kubectl apply -f k8s/` for all 5 services. **All 5 pods
  reached `1/1 Running` within ~12 seconds.** Then ran real traffic
  through it via `kubectl port-forward`: `POST /nodes` and `GET
  /nodes/:id` through the gateway (auth enforced, request proxied
  correctly), `POST /extract-and-store` through the extraction service
  (correctly produced and stored real nodes/edges — `/stats` went from
  2 to 4 nodes), `POST /rank` against the ranking service directly, and
  confirmed `X-Cache: HIT` on a repeated node GET — **caching actually
  worked inside the cluster**, over real pod-to-pod networking via
  Kubernetes Service DNS names (`cortexkernel-redis`,
  `cortexkernel-storage`), not just docker-compose's flatter network.
  Found one honest, real thing along the way: storage's startup log
  showed `redis_reachable="false"` because its pod started and pinged
  Redis before Redis's own pod had finished coming up — a real
  Kubernetes startup-ordering issue, not a bug in the caching logic
  itself (confirmed caching worked fine moments later, once Redis was
  actually ready — see the design-decision entry above on this). Cluster
  was deleted afterward (`kind delete cluster`) — this is not left
  running, don't assume it exists in future sessions without recreating it.
- Attempted Terraform for local docker orchestration (parity with
  `docker-compose.yml` but as IaC) and it's currently blocked: Homebrew
  removed `terraform` from homebrew-core over licensing, and the
  HashiCorp tap's bottle needs a newer Xcode Command Line Tools version
  than is installed on this machine — fixing that means a system
  software update, which this session isn't going to push through
  unprompted. Flagging so a future session doesn't waste time
  rediscovering this; ask Surya to update CLT first, or just skip
  Terraform and keep `docker-compose.yml` as the local-orchestration
  story (they overlap in purpose for local dev).
- **Backend additions for the frontend, verified**: `GraphStore::all_nodes()`
  plus `GET /nodes` and `GET /edges` (list-all), and permissive CORS
  (`Access-Control-Allow-Origin: *` + a catch-all `OPTIONS` preflight
  handler). All verified by hand: seeded two `lives_in` facts (NYC and
  LA) for `self`, confirmed `GET /nodes`/`GET /edges` return the full
  set, confirmed `/contradictions` correctly flags the NYC/LA conflict,
  confirmed a real `OPTIONS` preflight returns `204` with the right
  `Access-Control-*` headers.
- **Frontend (`web/`): built, type-checked, and actually run — not just
  written.** `npm install` + `npm run build` (`tsc -b && vite build`)
  both succeeded for real (one real TypeScript error was caught and
  fixed along the way: D3's `.selectAll('circle')` needed an explicit
  `<SVGCircleElement, SimNode>` type parameter, otherwise `.call(drag)`
  didn't type-check — a genuine type-safety catch, not a formality).
  `npm run dev` (Vite) was started for real and confirmed serving
  `index.html` and every `.tsx` module correctly. **What was NOT done**:
  no headless browser was available in this session to screenshot the
  actual rendered DOM/SVG, so the *rendering* itself (React mounting,
  D3 drawing circles/links, the drag behavior working) is verified only
  by (a) a clean type-checked build and (b) confirming the exact JSON
  shape the API returns matches what `api.ts`'s TypeScript interfaces
  expect, fed with real contradiction data from a live `storage_server`.
  That's strong but not the same as "someone looked at it in a browser."
  Worth actually opening it in a browser next session to eyeball-verify.
- **Found and worked around an unrelated local port conflict, not a
  CortexKernel bug**: port 8080 was occupied by an unrelated project on
  this machine (`~/GitHub Projects/MarketsBot`, its `mbot dashboard
  --port 8080` process), which silently absorbed every curl request
  during testing and made it look like `storage_server` was completely
  broken (every route 404ing, including `/health`) — it wasn't; the
  real server was simply failing to bind port 8080 and never actually
  received the requests. Diagnosed with `lsof -nP -iTCP:8080
  -sTCP:LISTEN`. Worked around it for this session's testing by
  temporarily building and running a copy on port 18888 rather than
  killing someone else's unrelated running process. **If `storage_server`
  seems to not respond or 404s on everything in a future session, check
  for this before assuming the code broke** — it didn't, last time.
- **Documentation: full rewrite done.** `README.md`, `LICENSE`,
  `CHANGELOG.md`, and `docs/{ARCHITECTURE,CONTRADICTION_DETECTION,
  BENCHMARKS,API,DEPLOYMENT}.md`. Every fact and number in these was
  pulled from this file's own prior Current-status entries — nothing was
  invented or re-estimated for the docs. Verified the test counts quoted
  (90 C++ assertions / 33 test cases, 28 Python `unittest` cases across
  ranking/gateway/extraction) by actually re-running all four suites
  fresh before writing them down, rather than trusting older text in
  this file that predated the extraction service's tests. Not
  independently verified: that the two Mermaid diagrams
  (`README.md`, `docs/ARCHITECTURE.md`) actually render — no Mermaid
  renderer was available in this session to check syntax beyond careful
  manual review; GitHub and most Markdown viewers render Mermaid
  natively, so check this the first time either file is viewed on
  GitHub.
- **gRPC + Protocol Buffers between extraction and storage: built and
  verified thoroughly, in two separate environments.** `protobuf` and
  `grpc` installed via Homebrew (`brew install protobuf grpc`) —
  bottled, no compilation, unlike Terraform's earlier CLT problem.
  `proto/cortexkernel.proto` defines `AddNode`/`AddEdge`/`GetNode`.
  `GrpcStorageService` runs on a background thread inside the same
  `storage_server` process as the REST API, sharing the same
  `GraphStore`, `WalWriter`, and `NodeCache`.
  - **C++ side, native (Homebrew/macOS)**: `cmake -B build` found
    protobuf/gRPC via `find_package(... CONFIG)`, generated the C++
    stubs, and built cleanly. 4 new Catch2 tests
    (`test_grpc_server.cpp`) each start a real in-process `grpc::Server`
    on an ephemeral port and connect a real client channel — no
    mocking. All pass: AddNode+GetNode round-trip, GetNode on a missing
    id, AddNode rejecting an invalid type with a real
    `INVALID_ARGUMENT` status, AddEdge. Full suite: **104 C++ assertions
    across 37 test cases**, up from 90/33.
  - **Python side**: generated `cortexkernel_pb2.py`/`cortexkernel_pb2_grpc.py`
    via `grpc_tools.protoc`, wrote `grpc_client.py`. Ran a real Python
    client against the real running gRPC server: `AddNode` then
    `GetNode` round-tripped correctly, and — the actual point of this
    integration — a node added via gRPC was confirmed retrievable via
    the REST API (`GET /nodes/:id`) and present in the WAL file,
    proving both protocols share one backend, not two siloed graphs.
  - **Docker/Linux side, verified separately and it did NOT work on the
    first attempt — real problems, really fixed:**
    1. `find_package(... CONFIG)` found nothing on Ubuntu — apt's
       `libgrpc++-dev` ships no CMake config package. Fixed by adding a
       `pkg-config` fallback path in `CMakeLists.txt` (confirmed via a
       throwaway `ubuntu:22.04` container that `grpc++.pc`/`protobuf.pc`
       exist even though no `gRPCConfig.cmake` does).
    2. The storage Dockerfile's build context was `./services/storage`,
       which can't see `../../proto/`. Fixed by changing
       `docker-compose.yml`'s storage service to `context: .` (repo
       root) with an explicit `dockerfile:` path, and adding a
       repo-root `.dockerignore` so the wider context doesn't pull in
       `.git`, `node_modules`, etc.
    3. The built container crashed on startup:
       `error while loading shared libraries: libgrpc++.so.1`. The
       slim runtime stage only had `libstdc++6`. Fixed by adding the
       actual runtime shared-lib packages (`libgrpc++1`, `libgrpc10`,
       `libprotobuf23`, `libssl3`, `libc-ares2`, `zlib1g`) — found via
       `apt-cache search` in a throwaway container, not guessed.
    4. The Python side had a separate real bug: `cortexkernel_pb2.py`
       was generated with `protobuf` 7.36.1 (whatever was locally
       installed), but `requirements.txt` pinned `protobuf<6` for the
       container — a genuine `VersionError: Detected incompatible
       Protobuf Gencode/Runtime versions` at import time inside the
       built image. Caught by actually running
       `python3 -c "import grpc_client"` inside the container, not by
       assuming pip install succeeding meant it worked. Fixed by
       pinning `requirements.txt` to `protobuf>=7,<8` to match what
       was actually used for codegen.
    After all four fixes: rebuilt the storage image, ran it as a real
    container, confirmed `/health` (REST) and a real Python gRPC client
    calling `AddNode`/`GetNode` against `localhost:<mapped-50051>` both
    worked, and confirmed the gRPC-added node was visible via the
    container's REST API too. Full details and the exact fixes in
    `docs/GRPC.md`.
  - **Known, documented inconsistency**: nodes created via gRPC get an
    empty `layer` field; REST-created nodes default it to `"life"`
    (a `json_translation.cpp` DTO default that `grpc_server.cpp`
    doesn't share, since it builds a `Node` directly rather than going
    through `node_from_json()`). Minor, real, flagged in `docs/GRPC.md`
    — not fixed this session.
  - **What's still not done**: no gRPC coverage for
    `/contradictions`/`/drift`; `grpc_client.py` exists but nothing
    calls it from `app.py` yet; no TLS (insecure credentials only, fine
    for local dev).
- **Semantic value/behavior mismatch detection: built, tested, exposed
  over HTTP, and verified.** `find_semantic_value_behavior_mismatches()`
  embeds `predicate + " " + object_name` (existing `embed_text()`/
  `SemanticIndex` machinery) for live `StatedValue`/`BehaviorEvidence`
  edge pairs sharing a subject and flags pairs above a cosine-similarity
  threshold — catching mismatches that don't share an exact predicate or
  object, which the original exact-match detector can't. 5 new Catch2
  tests (110 assertions / 42 test cases total now, up from 104/37).
  Exposed at `GET /contradictions/semantic?threshold=0.5`, verified over
  real HTTP: two edges pointing at differently-`object_id`'d but
  identically-named ("personal health") nodes were correctly flagged,
  and a `threshold=1.1` query correctly returned nothing. **Be precise
  in any future write-up**: this is text-similarity on a placeholder
  embedding, not real semantic understanding — see the design-decision
  entry above and `docs/CONTRADICTION_RANKING.md`.
- **The RL ranking loop is closed, end to end, verified with three live
  processes plus a real script run against docker-compose — this had
  been flagged as "not wired into any real flow" in every single prior
  round.** `services/extraction/ranking_client.py` converts storage's
  `/contradictions` output into the bandit's `{id, category, confidence}`
  shape, calls ranking's `/rank`, and re-orders the original
  contradiction objects by the result; `submit_feedback()` forwards to
  `/feedback`. New endpoints on `app.py`: `GET /contradictions/ranked`,
  `POST /feedback`. **Verified twice, not once**:
  1. Ran `storage_server`, `ranking/service.py`, and `extraction/app.py`
     as three real native processes. Seeded a direct contradiction and a
     value/behavior one. Called `/contradictions/ranked` (both present).
     Submitted feedback favoring `value_behavior` over `direct`. Called
     `/contradictions/ranked` again — **the order actually flipped**,
     confirmed in the raw JSON response, not inferred from reading code.
  2. Wrote `scripts/demo.sh`, a one-command `docker compose up` +
     curl-driven walkthrough of the same flow. **It did not work
     correctly on the first two runs** — worth recording exactly why:
     first run only produced one contradiction category (the mock
     extractor's "I value X"/"I worked on Y" produces `values`/`did` as
     different predicates, so that pair never hits the exact-match
     detector at all), so there was nothing to visibly reorder; fixed by
     also seeding a same-predicate `StatedValue`/`BehaviorEvidence` pair
     directly via the gateway. Second run showed two categories but the
     feedback happened to favor whichever one was already ranked first
     by tie-breaking (both edges had `confidence=1.0`, and the bandit's
     value estimates start at 0 for everyone, so ties broke by whatever
     order storage's `/contradictions` endpoint happened to return them
     in), so still no visible change; fixed by feeding reward to the
     category that *wasn't* already first. Third run showed the flip.
     The script now also documents, for anyone who runs it, that the
     bandit explores randomly ~10% of the time by design (unseeded
     `EpsilonGreedyRanker(epsilon=0.1)` in `service.py`), so an
     occasional non-flip on a given run is expected behavior, not a bug.
  3. Wired the frontend into the same loop: `App.tsx` now calls
     `fetchRankedContradictions()` instead of the raw `fetchContradictions()`,
     and each item gets a new `FeedbackButtons` component
     ("Useful"/"Dismiss") wired to `submitFeedback()` + a re-fetch.
     Verified via a clean `tsc -b && vite build` only — no headless
     browser available to actually click the buttons and watch the DOM
     update, same caveat as the original frontend work.
- New Python test coverage this round: `test_grpc_client.py` (4 tests,
  closing a real gap — `grpc_client.py` had zero tests before this),
  `test_ranking_client.py` (3 tests), `test_app_ranking.py` (3 tests,
  and building it surfaced a real mocking bug — patching `app.httpx.Client`
  with a replacement that itself called `httpx.Client(...)` created
  infinite self-reference, since `app.httpx` and the top-level `httpx`
  import are the same module object; fixed with a plain `MagicMock`).
  Full Python count: 38 `unittest` cases across ranking/gateway/extraction,
  up from 28.

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
12. ~~Extraction service: exposes its own HTTP API, containerized~~ — DONE
    (`services/extraction/`), verified end-to-end with a real running
    storage service and again over `docker-compose`. **What's actually
    NOT done**: the real Anthropic-backed extraction path
    (`AnthropicExtractionClient`) is written but has never been run — no
    API key available this session. The service defaults to
    `MockExtractionClient` (a real, working, regex-based heuristic
    extractor), which is what was verified. Do the Anthropic path with
    Surya present to supply a key, and expect the prompt/parsing to need
    at least one iteration — LLM output not perfectly matching the
    expected JSON schema is the realistic failure mode.
13. ~~Wire `SemanticIndex` into `ContradictionDetector`~~ — DONE
    (`find_semantic_value_behavior_mismatches()`, exposed at
    `GET /contradictions/semantic`). **Still open, and this is the part
    that actually matters**: swap `embed_text()`'s hashed-trigram
    placeholder for real embeddings (an actual embedding model or API
    call) — NOT STARTED, blocked on the same missing API key as item 12.
    Until that happens, the wiring is real but what it catches is
    text-similarity, not semantic equivalence — don't overstate this on
    a resume or in conversation; see the design-decision entry on this.
14. ~~Persist the bandit's learned value estimates~~ — DONE
    (`persistence.py`, wired into `service.py`, verified across a real
    restart). ~~Add a Docker entry~~ — DONE. ~~Actually call
    `service.py`'s `POST /rank`/`POST /feedback` from somewhere real~~ —
    **DONE.** `ranking_client.py` + new `app.py` endpoints
    (`GET /contradictions/ranked`, `POST /feedback`) close this loop for
    real — verified with three live processes (the ranking actually
    flipped after feedback) and again via `scripts/demo.sh` against
    docker-compose. See Current status for the two demo-script bugs
    found and fixed along the way (not everything worked on the first
    try, and that's recorded honestly rather than glossed over).
15. ~~gRPC + Protocol Buffers~~ — DONE for nodes/edges (`AddNode`,
    `AddEdge`, `GetNode`), verified both natively (Homebrew, in-process
    Catch2 tests against a real server) and inside Docker (a real
    version-mismatch bug and a real missing-shared-libs bug, both found
    and fixed — see Current status and `docs/GRPC.md`). **Still open**:
    no gRPC path for contradictions/drift, and `grpc_client.py` isn't
    called from `app.py`'s actual extraction flow yet (it calls storage
    over REST, not gRPC) — it's available,
    not integrated.
16. ~~Redis: caching layer for storage's hot read paths~~ — DONE
    (`RedisClient` + `NodeCache`, wired into `POST/GET /nodes`), verified
    against a real local Redis and again over real docker-compose
    networking, including graceful degradation with Redis down. **Still
    open from this item**: an event stream (Redis Streams/NATS) for async
    extraction → storage ingestion — not attempted, there's no extraction
    service producing events to stream yet (blocked on item 12).
17. ~~Lightweight API gateway in front of storage: API-key auth, rate
    limiting~~ — DONE (`services/gateway/`), verified against a real
    `storage_server` process AND through `docker-compose.yml` with real
    container-to-container networking. Not yet in front of the
    extraction/ranking services too, since there's no real traffic to
    those yet — revisit once item 12 exists.
18. ~~`kind` cluster set up locally + deploy all services~~ — **DONE, for
    real.** All 5 services (storage, redis, ranking, extraction, gateway)
    deployed to an actual local `kind` cluster, all 5 pods reached
    `1/1 Running`, and real traffic was run through it (gateway auth +
    proxy, extraction writing real data into storage, caching actually
    hitting Redis over Kubernetes networking) — see Current status for
    the full transcript. The cluster itself was deleted afterward
    (`kind delete cluster`) since it's not meant to be left running
    between sessions; recreating it is `kind create cluster --name
    cortexkernel && docker build ... && kind load docker-image ... &&
    kubectl apply -f k8s/`. The Helm chart (item 19) has not been applied
    to a cluster yet — only the raw `k8s/` manifests were used for this
    verification.
19. ~~Helm chart packaging~~ — DONE (`helm/cortexkernel/`), `helm lint` +
    `helm template` verified. Same "not applied to a real cluster"
    caveat as the raw manifests in item 18.
20. Terraform for cluster/resource provisioning — **blocked**, not
    started: Homebrew's `terraform` formula is gone from homebrew-core,
    and the HashiCorp tap's bottle needs newer Xcode Command Line Tools
    than are installed here. Needs a CLT update (a real system change,
    not something to do unprompted) before this is worth attempting again.
21. ~~Prometheus metrics endpoint + Grafana dashboard~~ — **DONE and
    fully verified**, not just configured: `docker-compose.yml` now runs
    `prometheus` (scraping storage's real `/metrics`) and `grafana` (auto-
    provisioned datasource + dashboard, both confirmed live via Grafana's
    own API). Known, accepted limitation recorded above: metrics record
    raw paths, not templated ones (cardinality risk at real scale).
    **Still open**: no OpenTelemetry tracing, and the dashboard only
    covers raw HTTP request metrics — it doesn't yet include the bandit's
    per-category value estimates or the detector's benchmarked latencies
    as tracked metrics (both would need new `/metrics`-exposed counters
    first, they don't emit Prometheus-format data today).

**Tier 4 — presentation, do last:**
22. ~~React + TypeScript + D3.js web UI visualizing the graph~~ — DONE,
    built and type-checked (see Current status for the exact verification
    and its one honest gap — never opened in an actual browser this
    session). ~~Surface contradictions ranked by the bandit~~ — DONE:
    `App.tsx` calls extraction's `/contradictions/ranked` instead of
    storage's raw `/contradictions`, and `ContradictionsPanel` has
    `FeedbackButtons` wired to `submitFeedback()` + a re-fetch. Same
    "never clicked in an actual browser" caveat as the rest of the
    frontend applies here too.
23. ~~Full README rewrite~~ — DONE: `README.md` (badges, Mermaid diagram,
    quickstart, tech stack table), plus `LICENSE` (MIT), `CHANGELOG.md`
    (built from real `git log`, not invented), and a `docs/` folder
    (`ARCHITECTURE.md`, `CONTRADICTION_DETECTION.md`, `BENCHMARKS.md`,
    `API.md`, `DEPLOYMENT.md`). Every number in `BENCHMARKS.md` was
    pulled from this file's own Current status entries, not re-derived
    or estimated — including the honest small-scale regression the O(n²)
    fix introduces at 100 edges, which the docs report rather than hide.
    **Still open**: the "semantic search vs. exact match" comparison
    mentioned in the old version of this item doesn't exist yet, because
    `SemanticIndex` still isn't wired into `ContradictionDetector` (see
    item 13) — there's nothing to benchmark yet. Don't add that section
    to the docs until item 13 is actually done.

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
