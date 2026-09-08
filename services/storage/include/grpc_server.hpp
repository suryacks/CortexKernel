#pragma once

#include "cortexkernel.grpc.pb.h"
#include "graph_store.hpp"
#include "node_cache.hpp"
#include "persistence.hpp"

namespace kg {

class GrpcStorageService final : public cortexkernel::CortexKernelStorage::Service {
public:
    GrpcStorageService(GraphStore& store, WalWriter& wal, NodeCache& node_cache);

    grpc::Status AddNode(grpc::ServerContext* context, const cortexkernel::NodeRequest* request,
                         cortexkernel::NodeReply* reply) override;
    grpc::Status AddEdge(grpc::ServerContext* context, const cortexkernel::EdgeRequest* request,
                         cortexkernel::EdgeReply* reply) override;
    grpc::Status GetNode(grpc::ServerContext* context, const cortexkernel::GetNodeRequest* request,
                         cortexkernel::NodeReply* reply) override;

private:
    GraphStore& store_;
    WalWriter& wal_;
    NodeCache& node_cache_;
};

}
