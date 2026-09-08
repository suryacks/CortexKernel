# API reference

Every example below was actually run against a live service during
development, not written from the source alone.

## Storage service (`services/storage`, default port 8080)

### `GET /health`

```bash
curl http://localhost:8080/health
# {"status":"ok"}
```

### `POST /nodes`

```bash
curl -X POST http://localhost:8080/nodes \
  -d '{"id":"self","type":"Self","name":"Self"}'
# {"created_at":"","id":"self","layer":"life","name":"Self","source_ref":"","type":"Self","updated_at":""}
```

`type` must be one of: `Self`, `Person`, `Place`, `Org`, `Project`,
`Concept`, `Value`, `Event`, `Emotion`, `Artifact`, `Task`.

### `GET /nodes/:id`

```bash
curl http://localhost:8080/nodes/self
```

Returns `404` if the node doesn't exist. Response carries an
`X-Cache: HIT` or `X-Cache: MISS` header (Redis cache-aside).

### `GET /nodes`

Lists every node.

### `POST /edges`

```bash
curl -X POST http://localhost:8080/edges \
  -d '{"id":"e1","from":"self","predicate":"lives_in","to":"nyc","edge_class":"Fact"}'
```

`edge_class` must be one of: `StatedValue`, `BehaviorEvidence`, `Fact`,
`Emotion`, `Reflection`. `from`/`to` are node IDs.

### `GET /edges`

Lists every edge (live and invalidated).

### `GET /nodes/:id/edges`

All edges where the node is the subject or the object.

### `GET /stats`

```bash
curl http://localhost:8080/stats
# {"node_count":3,"edge_count":2}
```

### `GET /contradictions`

Runs both `find_direct_contradictions()` and
`find_value_behavior_mismatches()` and returns them together:

```bash
curl http://localhost:8080/contradictions
```

```json
[
  {
    "type": "direct",
    "subject_id": "self",
    "predicate": "lives_in",
    "edge_a": { "...": "the nyc edge" },
    "edge_b": { "...": "the la edge" }
  }
]
```

### `GET /drift/:subject_id/:predicate`

```bash
curl http://localhost:8080/drift/self/lives_in
# {"predicate":"lives_in","state":"CONTRADICTED","subject_id":"self"}
```

`state` is one of `HELD`, `REFINED`, `CONTRADICTED`, `BOTH`,
`SUPERSEDED` — see [CONTRADICTION_DETECTION.md](./CONTRADICTION_DETECTION.md).
Returns `404` if there's no edge history for that subject+predicate.

### `GET /metrics`

Prometheus exposition format — request counts and duration sums per
`method`/`path`/`status`. See [BENCHMARKS.md](./BENCHMARKS.md) and the
Grafana dashboard in `observability/grafana/dashboards/cortexkernel.json`.

CORS is wide open (`Access-Control-Allow-Origin: *`) on every endpoint
above, including an `OPTIONS` preflight handler, so the web UI (or
anything else running in a browser) can call storage directly.

## Gateway (`services/gateway`, default port 8090)

The public entry point. Every request needs an `X-API-Key` header
matching one of the keys in `CORTEXKERNEL_API_KEYS` (comma-separated,
default `dev-key`). Requests are rate-limited per key (token bucket,
configurable via `RATE_LIMIT_CAPACITY`/`RATE_LIMIT_REFILL_PER_SEC`).
Everything else is proxied straight through to storage — same paths as
above, just through the gateway:

```bash
curl -H "X-API-Key: dev-key" http://localhost:8090/health
curl -H "X-API-Key: dev-key" -X POST http://localhost:8090/nodes -d '{"id":"n1","type":"Person","name":"test"}'
```

No key or a wrong key: `401`. Over the rate limit: `429`. Storage
unreachable: `502`.

## Extraction service (`services/extraction`, default port 8082)

### `GET /health`

### `POST /extract`

Runs the configured backend (`EXTRACTION_BACKEND=mock` by default, or
`anthropic` — see [ARCHITECTURE.md](./ARCHITECTURE.md)) and returns
candidates without storing them:

```bash
curl -H "Content-Type: application/json" -X POST http://localhost:8082/extract \
  -d '{"text":"I value my health. I worked on the project all night.","source_ref":"journal-1"}'
```

### `POST /extract-and-store`

Same extraction, then POSTs every resulting node and edge to the
storage service (`STORAGE_URL` env var) and returns what storage stored:

```bash
curl -H "Content-Type: application/json" -X POST http://localhost:8082/extract-and-store \
  -d '{"text":"I value my health. I worked on the project all night.","source_ref":"journal-1"}'
```

This is what turns free text into real graph data — see the mock
extractor's patterns in `mock_extraction_client.py` for exactly what
phrasing it currently recognizes.

## Ranking service (`services/extraction/ranking`, default port 8081)

### `POST /rank`

```bash
curl -X POST http://localhost:8081/rank \
  -d '{"contradictions":[{"id":"a","category":"direct","confidence":0.5},{"id":"b","category":"value_behavior","confidence":0.5}]}'
```

Returns the same list, re-ordered by the bandit's learned per-category
value plus confidence.

### `POST /feedback`

```bash
curl -X POST http://localhost:8081/feedback -d '{"category":"value_behavior","reward":1.0}'
```

Updates the bandit's estimate for that category and persists it to
`bandit_state.json` immediately (survives a restart).
