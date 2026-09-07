# CortexKernel — Project Context for Claude Code

## What this is

CortexKernel is a **distributed, bi-temporal knowledge graph platform** for
tracking a person's stated values against their observed behavior over
time — built as a portfolio/resume project demonstrating systems
programming (C++), distributed systems (gRPC, event-driven pipelines),
ML/NLP engineering (Python, LLM-based extraction), data infrastructure
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
        httplib.h                — vendored single-header HTTP library
        json.hpp                 — vendored nlohmann/json single header
      src/
        graph_store.cpp
        contradiction_detector.cpp
        json_translation.cpp
        main.cpp                 — HTTP server entrypoint (DONE)
      tests/
        test_graph_store.cpp      — Catch2 unit tests (DONE, passing)
        test_json_translation.cpp — Catch2 unit tests (DONE, passing)
        test_contradiction_detector.cpp — NOT STARTED
      CMakeLists.txt              — FetchContent for Catch2 (DONE)
      Dockerfile                  — multi-stage build (NOT STARTED)
    extraction/                  — Python LLM-based extraction pipeline (NOT STARTED)
      Calls the Anthropic API to turn raw journal/chat text into typed
      Node/Edge candidates, posts them to the storage service's REST API.
      Planned stack: FastAPI, Pydantic, httpx.
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
- **Benchmarking (planned)**: load-testing suite (k6 or wrk) producing
  real latency/throughput numbers vs. a naive SQLite baseline, published
  in the README.

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
  (HELD / REFINED / CONTRADICTED / BOTH / SUPERSEDED) implemented and
  compiling against the existing `GraphStore`. **Not yet unit tested —
  next session should add `test_contradiction_detector.cpp` before
  building anything else on top of it.**
- Dockerfile for storage service: not started.
- README: not started.
- Nothing outside `services/storage` started yet.

## Roadmap (prioritized, in order)

**Tier 1 — finish the storage service:**
1. ~~Real JSON API~~ — DONE
2. ~~Unit tests + CI~~ — DONE
3. ~~Contradiction-detection core (direct + value/behavior + drift)~~ —
   DONE, needs tests
4. Catch2 tests for `ContradictionDetector` — NOT STARTED
5. Structured logging (spdlog) replacing `std::cout` in main.cpp — NOT STARTED
6. Persistence — durable storage so `GraphStore` survives restarts
   (currently pure in-memory) — NOT STARTED

**Tier 2 — prove it with numbers:**
7. Benchmark/comparison writeup vs. a naive SQLite baseline — real
   numbers (query latency, memory footprint, req/sec) for the README.
8. Expose contradiction/drift results over the HTTP API
   (`GET /contradictions`, `GET /drift/:subject_id/:predicate`).

**Tier 3 — distributed systems + infra polish:**
9. Extraction service: Python, calls the Anthropic API for entity/relation
   extraction, exposes its own HTTP API, containerized with the same
   multi-stage Docker pattern as storage.
10. gRPC + Protocol Buffers between extraction and storage (in addition
    to the public REST API).
11. Redis: caching layer for storage's hot read paths, plus an event
    stream (Redis Streams/NATS) for async extraction → storage ingestion.
12. Lightweight API gateway in front of both services: API-key auth,
    rate limiting.
13. `kind` cluster + Kubernetes manifests deploying storage + extraction
    + gateway as separate pods talking over k8s Services.
14. Helm chart packaging (replacing raw k8s YAML).
15. Terraform for cluster/resource provisioning.
16. Prometheus metrics endpoint + Grafana dashboard + OpenTelemetry tracing.

**Tier 4 — presentation, do last:**
17. React + TypeScript + D3.js web UI visualizing the graph and
    surfacing detected contradictions.
18. Full README rewrite: architecture diagram (Mermaid), badges, "why I
    built this," benchmark numbers front and center.

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
