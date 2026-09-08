# Changelog

A narrative summary of how this project actually got built, grouped by
milestone rather than by individual commit. See `git log` for the exact
commit-by-commit history.

## Storage service foundation

In-memory `GraphStore`, the bi-temporal `Node`/`Edge` data model, a JSON
HTTP API over `httplib`, a DTO layer decoupling the external JSON shape
from internal struct fields, Catch2 unit tests, and GitHub Actions CI.

## Contradiction detection (the core idea)

`ContradictionDetector`: direct contradictions, value/behavior
mismatches, and the five-state drift classifier (`HELD` / `REFINED` /
`CONTRADICTED` / `BOTH` / `SUPERSEDED`). Fully unit tested. See
[docs/CONTRADICTION_DETECTION.md](docs/CONTRADICTION_DETECTION.md).

## Durability, logging, and the ML/RL/perf detour

- WAL-based persistence (`WalWriter` + replay), wired into `main.cpp`
  and verified across a real process restart.
- A dependency-free structured logger (chosen over `spdlog` to avoid an
  unaudited new dependency).
- `SemanticIndex`: a cosine-similarity vector index with a placeholder
  hashed-trigram embedding — the first step toward semantic (not just
  exact-match) contradiction detection.
- `EpsilonGreedyRanker`: a multi-armed bandit that learns which category
  of contradiction is worth surfacing first, with JSON-file persistence
  for its learned state.
- **Found and fixed a real O(n²) performance bug** in the contradiction
  detector via a purpose-built benchmark — see
  [docs/BENCHMARKS.md](docs/BENCHMARKS.md) for the ~47x before/after
  numbers, and a naive-SQLite comparison showing the fixed version is
  ~35x faster than the query most people would write first.

## Distributed system

- `GET /contradictions` and `GET /drift/:subject_id/:predicate` exposed
  over HTTP.
- An API gateway (`services/gateway`): `X-API-Key` auth plus per-key
  token-bucket rate limiting, proxying to storage. Unit tests here
  caught a real bug (a clock-seeding mismatch in the rate limiter) before
  it shipped.
- Redis-backed cache-aside node caching, via a from-scratch RESP client
  (no `hiredis`) — verified against a real Redis, and verified to fail
  soft (never breaks the service) when Redis is down.
- A Prometheus `/metrics` endpoint.
- `docker-compose.yml` bringing up all services together on one network.

## Extraction and Kubernetes

- `services/extraction`: a FastAPI service with two interchangeable
  backends — a real, working regex-based mock extractor (the default,
  zero dependencies) and an Anthropic-backed extractor (written, not yet
  run — no API key available during development). Verified end-to-end:
  real journal text in, real nodes/edges landing in storage.
- Helm chart for the storage service (`helm lint`/`helm template`
  verified).
- Full Prometheus + Grafana observability stack, with a real dashboard
  built against the actual exposed metric names, auto-provisioned.
- **Deployed the entire stack to a real local Kubernetes cluster**
  (`kind`): all 5 services, all 5 pods reaching `1/1 Running`, real
  traffic proxied through the gateway and real caching verified over
  Kubernetes pod-to-pod networking.

## Frontend

A React + TypeScript + D3 web UI (`web/`): a force-directed graph view
(nodes involved in a detected contradiction rendered in red) and a
contradictions list, both fed by the storage API directly (CORS enabled
for this). Type-checked and built successfully; not yet eyeballed in an
actual browser.

## What's still open

Tracked in detail in [`CLAUDE.md`](CLAUDE.md), which is the working log
this project is actually built from. In short: the real Anthropic
extraction path and real embeddings are blocked on an API key; gRPC and
Terraform haven't been started (Terraform specifically is blocked on an
Xcode Command Line Tools version); and the bandit ranker isn't wired
into any real flow yet.
