# gRPC

Storage exposes a second protocol alongside its REST API: gRPC on port
`50051`, defined in `proto/cortexkernel.proto` and shared by every
language that needs a client. This isn't a separate service or a
separate in-memory graph — it's the same `GraphStore`, `WalWriter`, and
`NodeCache` the REST API uses, running in the same process on a
background thread. A node added over gRPC is immediately visible over
REST, and vice versa; both paths write to the same write-ahead log.

## The service

```protobuf
service CortexKernelStorage {
  rpc AddNode(NodeRequest) returns (NodeReply);
  rpc AddEdge(EdgeRequest) returns (EdgeReply);
  rpc GetNode(GetNodeRequest) returns (NodeReply);
}
```

A deliberately small surface — parity with the REST API's core
read/write path (`POST /nodes`, `POST /edges`, `GET /nodes/:id`), not
the full API (no contradiction/drift endpoints over gRPC yet).

## Using it from Python

```python
from grpc_client import StorageGrpcClient
from schemas import NodeCandidate, NodeType

client = StorageGrpcClient("localhost:50051")
client.add_node(NodeCandidate(id="n1", type=NodeType.PERSON, name="Surya"))
reply = client.get_node("n1")
client.close()
```

`services/extraction/cortexkernel_pb2.py` and `cortexkernel_pb2_grpc.py`
are generated from `proto/cortexkernel.proto` and committed (Python has
no equivalent to CMake's build-time codegen story, so — like most
Python + gRPC projects — the generated stubs are checked in rather than
regenerated on every build). Regenerate them after changing the `.proto`:

```bash
python -m grpc_tools.protoc -I proto \
  --python_out=services/extraction --grpc_python_out=services/extraction \
  proto/cortexkernel.proto
```

**A real version-compatibility bug worth knowing about**: the first
attempt at this generated stubs using whatever `protobuf`/`grpcio-tools`
happened to be installed locally (7.36.1, quite new), then pinned
`requirements.txt` to an older, more conservative `protobuf<6` — which
broke at runtime with `VersionError: Detected incompatible Protobuf
Gencode/Runtime versions`. Protobuf enforces that the runtime can't be
older than the compiler that generated the code. Fixed by pinning
`requirements.txt`'s `protobuf` version to match what actually generated
the stubs, not what seemed conservative. If you regenerate the stubs
with a different `protobuf`/`grpcio-tools` version, update
`requirements.txt` to match, and verify by actually importing
`grpc_client` inside the built container — a nearly-good version range
isn't good enough here.

## Building it in C++

`services/storage/CMakeLists.txt` looks for `protobuf`/`gRPC` two ways:
first via `find_package(... CONFIG)` (works with Homebrew's builds on
macOS, which ship proper CMake config packages), then falls back to
`pkg-config` (works with Ubuntu/Debian's `libgrpc++-dev`, which only
ships `.pc` files, not CMake config packages — a real, documented gap in
how Debian packages gRPC). If neither is found, the whole gRPC service
is skipped at configure time and the REST API is unaffected —
`CORTEXKERNEL_GRPC_ENABLED` gates the code in `main.cpp` behind a
preprocessor guard, so `storage_server` still builds and runs
REST-only. This is why CI (which doesn't install protobuf/grpc) still
passes: it's building the REST-only configuration, on purpose.

Locally, verified two ways:

- Natively via Homebrew (`brew install protobuf grpc`): `cmake -B build
  && cmake --build build` picks it up via the `CONFIG` path, and 4 new
  Catch2 test cases (`tests/test_grpc_server.cpp`) start a real
  in-process `grpc::Server`, connect a real client channel to it, and
  assert on real RPC responses — including one asserting the server
  correctly rejects an invalid node type with `INVALID_ARGUMENT`.
- Inside Docker (`services/storage/Dockerfile`, now built from the
  repository root so it can see `proto/`): apt's `libgrpc++-dev` +
  `pkg-config` triggers the fallback path, and the resulting
  `storage_server` was actually run in a container with both `/health`
  (REST) and a live `AddNode`/`GetNode` round trip (gRPC, from a real
  Python client on the host) verified against it.

## A known inconsistency between the two ingestion paths

Nodes created via gRPC get an empty `layer` field, while nodes created
via REST default `layer` to `"life"` (set in `json_translation.cpp`'s
DTO layer). `grpc_server.cpp` builds a `Node` directly rather than going
through `node_from_json()`, so it doesn't inherit that default. Minor,
but real — worth fixing by sharing the same construction path (or at
least the same defaults) if this protocol sees more use.

## What's not done

- No gRPC coverage for `/contradictions` or `/drift/...`.
- `grpc_client.py` isn't called by `app.py` yet — it's available, not
  wired into the extraction pipeline's actual flow.
- No TLS — `InsecureServerCredentials()`/`InsecureChannelCredentials()`
  on both ends, fine for local dev, not for anything else.
