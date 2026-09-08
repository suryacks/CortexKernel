FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ ninja-build git ca-certificates libsqlite3-dev pkg-config \
    protobuf-compiler libprotobuf-dev libgrpc++-dev protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY proto proto
COPY services/storage services/storage

WORKDIR /src/services/storage

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target storage_server

FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 libgrpc++1 libgrpc10 libprotobuf23 libssl3 libc-ares2 zlib1g \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/services/storage/build/storage_server .

EXPOSE 8080 50051
ENTRYPOINT ["./storage_server"]
