# Running it

## Fastest path: docker-compose

```bash
docker compose up -d
```

Brings up all 7 services on one network: `redis`, `storage` (8080),
`ranking` (8081), `extraction` (8082), `gateway` (8090), `prometheus`
(9090), `grafana` (3000, anonymous admin access enabled for local dev).
Everything talks to everything else by compose service name
(`http://storage:8080`, not `localhost`).

```bash
curl -H "X-API-Key: dev-key" http://localhost:8090/health
curl -H "Content-Type: application/json" -X POST http://localhost:8082/extract-and-store \
  -d '{"text":"I value honesty.","source_ref":"demo"}'
open http://localhost:3000   # Grafana, datasource + dashboard auto-provisioned
```

`docker compose down` tears it down.

**Known local gotcha**: if the storage service seems completely dead —
every route 404s, including `/health` — something else on the machine
may already be bound to port 8080. This isn't hypothetical: it happened
during development (an unrelated local project's dashboard was squatting
on the port). Check with `lsof -nP -iTCP:8080 -sTCP:LISTEN` before
assuming the code is broken.

## Building and running one service by hand

```bash
cd services/storage
cmake -B build && cmake --build build
./build/storage_server
```

Storage reads `REDIS_HOST`/`REDIS_PORT` from the environment (defaults
to `127.0.0.1:6379`) and is fully functional with no Redis running at
all — caching just always misses.

Each Python service (`services/gateway`, `services/extraction`,
`services/extraction/ranking`) can be run directly:

```bash
cd services/gateway && python3 gateway.py
cd services/extraction && pip install -r requirements.txt && uvicorn app:app --port 8082
cd services/extraction/ranking && python3 service.py
```

## Kubernetes (local, via `kind`)

This has been done and verified for real — see the "Current status" log
in [`CLAUDE.md`](../CLAUDE.md) for the full transcript (all 5 pods
reaching `1/1 Running`, real traffic proxied through the gateway,
caching actually hitting Redis over Kubernetes networking).

```bash
kind create cluster --name cortexkernel

docker build -t cortexkernel-storage:latest ./services/storage
docker build -t cortexkernel-ranking:latest ./services/extraction/ranking
docker build -t cortexkernel-gateway:latest ./services/gateway
docker build -t cortexkernel-extraction:latest ./services/extraction

kind load docker-image cortexkernel-storage:latest cortexkernel-ranking:latest \
  cortexkernel-gateway:latest cortexkernel-extraction:latest --name cortexkernel

kubectl apply -f k8s/

kubectl get pods   # wait for all 5 to reach 1/1 Running

kubectl port-forward svc/cortexkernel-gateway 8090:8090
# in another shell:
curl -H "X-API-Key: dev-key" http://localhost:8090/health
```

Only `cortexkernel-gateway` is a `NodePort` Service; everything else is
`ClusterIP` (internal-only) — deliberately, since the gateway is the
intended single entry point.

Tear down with `kind delete cluster --name cortexkernel` — the cluster
is not meant to be left running between sessions.

### Helm, as an alternative to raw manifests

```bash
helm lint helm/cortexkernel
helm template my-release helm/cortexkernel   # inspect rendered YAML
helm install my-release helm/cortexkernel    # against a real cluster
```

`helm/cortexkernel/values.yaml` parameterizes the image, replica count,
and Redis host/port for the storage Deployment. The Helm chart currently
only covers storage — the raw `k8s/` manifests are what's actually been
deployed for the other four services.

## The web frontend

```bash
cd web
npm install
npm run dev          # Vite dev server, http://localhost:5173
```

Set `VITE_API_URL` to point it at a non-default storage URL (defaults to
`http://localhost:8080`). `npm run build` produces a static `dist/` you
can serve from anything (nginx, the gateway, a CDN) — it's plain static
files with no server-side requirements.
