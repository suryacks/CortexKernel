#include "../include/grpc_server.hpp"
#include "../include/json_translation.hpp"

#include <stdexcept>

namespace kg {

GrpcStorageService::GrpcStorageService(GraphStore& store, WalWriter& wal, NodeCache& node_cache)
    : store_(store), wal_(wal), node_cache_(node_cache) {}

grpc::Status GrpcStorageService::AddNode(grpc::ServerContext*, const cortexkernel::NodeRequest* request,
                                         cortexkernel::NodeReply* reply) {
    Node node;
    node.id = request->id();
    node.name = request->name();
    try {
        node.type = node_type_from_string(request->type());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    }

    store_.add_node(node);
    wal_.record_add_node(node);
    node_cache_.put(node);

    reply->set_found(true);
    reply->set_id(node.id);
    reply->set_type(request->type());
    reply->set_name(node.name);
    return grpc::Status::OK;
}

grpc::Status GrpcStorageService::AddEdge(grpc::ServerContext*, const cortexkernel::EdgeRequest* request,
                                         cortexkernel::EdgeReply* reply) {
    Edge edge;
    edge.id = request->id();
    edge.subject_id = request->from();
    edge.predicate = request->predicate();
    edge.object_id = request->to();
    try {
        edge.edge_class = edge_class_from_string(request->edge_class());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    }

    store_.add_edge(edge);
    wal_.record_add_edge(edge);

    reply->set_id(edge.id);
    reply->set_from(edge.subject_id);
    reply->set_predicate(edge.predicate);
    reply->set_to(edge.object_id);
    reply->set_edge_class(request->edge_class());
    return grpc::Status::OK;
}

grpc::Status GrpcStorageService::GetNode(grpc::ServerContext*, const cortexkernel::GetNodeRequest* request,
                                         cortexkernel::NodeReply* reply) {
    auto cached = node_cache_.get(request->id());
    if (cached.has_value()) {
        reply->set_found(true);
        reply->set_id(cached->id);
        reply->set_type(node_type_to_string(cached->type));
        reply->set_name(cached->name);
        return grpc::Status::OK;
    }

    const Node* node = store_.get_node(request->id());
    if (node == nullptr) {
        reply->set_found(false);
        return grpc::Status::OK;
    }

    node_cache_.put(*node);
    reply->set_found(true);
    reply->set_id(node->id);
    reply->set_type(node_type_to_string(node->type));
    reply->set_name(node->name);
    return grpc::Status::OK;
}

}
