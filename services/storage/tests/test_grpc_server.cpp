#include <catch2/catch_test_macros.hpp>
#include "../include/grpc_server.hpp"
#include "../include/graph_store.hpp"
#include "../include/node_cache.hpp"
#include "../include/persistence.hpp"
#include "../include/redis_client.hpp"

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

namespace {

struct GrpcTestFixture {
    kg::GraphStore store;
    kg::RedisClient redis_client{"127.0.0.1", 1};
    kg::NodeCache node_cache{redis_client, 60};
    kg::WalWriter wal{"/tmp/cortexkernel_grpc_test.wal"};
    kg::GrpcStorageService service{store, wal, node_cache};
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<cortexkernel::CortexKernelStorage::Stub> stub;

    GrpcTestFixture() {
        int port = 0;
        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        stub = cortexkernel::CortexKernelStorage::NewStub(
            grpc::CreateChannel("127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials()));
    }

    ~GrpcTestFixture() {
        server->Shutdown();
    }
};

}

TEST_CASE("gRPC AddNode then GetNode round-trips a node", "[grpc]") {
    GrpcTestFixture fixture;

    cortexkernel::NodeRequest add_request;
    add_request.set_id("n1");
    add_request.set_type("Person");
    add_request.set_name("Surya");

    cortexkernel::NodeReply add_reply;
    grpc::ClientContext add_context;
    grpc::Status add_status = fixture.stub->AddNode(&add_context, add_request, &add_reply);

    REQUIRE(add_status.ok());
    REQUIRE(add_reply.found());
    REQUIRE(add_reply.id() == "n1");

    cortexkernel::GetNodeRequest get_request;
    get_request.set_id("n1");
    cortexkernel::NodeReply get_reply;
    grpc::ClientContext get_context;
    grpc::Status get_status = fixture.stub->GetNode(&get_context, get_request, &get_reply);

    REQUIRE(get_status.ok());
    REQUIRE(get_reply.found());
    REQUIRE(get_reply.name() == "Surya");
    REQUIRE(get_reply.type() == "Person");
}

TEST_CASE("gRPC GetNode on a missing id returns found=false", "[grpc]") {
    GrpcTestFixture fixture;

    cortexkernel::GetNodeRequest request;
    request.set_id("does-not-exist");
    cortexkernel::NodeReply reply;
    grpc::ClientContext context;
    grpc::Status status = fixture.stub->GetNode(&context, request, &reply);

    REQUIRE(status.ok());
    REQUIRE_FALSE(reply.found());
}

TEST_CASE("gRPC AddNode rejects an invalid node type", "[grpc][error-handling]") {
    GrpcTestFixture fixture;

    cortexkernel::NodeRequest request;
    request.set_id("n1");
    request.set_type("NotARealType");
    request.set_name("x");

    cortexkernel::NodeReply reply;
    grpc::ClientContext context;
    grpc::Status status = fixture.stub->AddNode(&context, request, &reply);

    REQUIRE_FALSE(status.ok());
    REQUIRE(status.error_code() == grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_CASE("gRPC AddEdge stores an edge retrievable via the same GraphStore", "[grpc]") {
    GrpcTestFixture fixture;

    cortexkernel::EdgeRequest request;
    request.set_id("e1");
    request.set_from("a");
    request.set_predicate("relates_to");
    request.set_to("b");
    request.set_edge_class("Fact");

    cortexkernel::EdgeReply reply;
    grpc::ClientContext context;
    grpc::Status status = fixture.stub->AddEdge(&context, request, &reply);

    REQUIRE(status.ok());
    REQUIRE(reply.id() == "e1");
    REQUIRE(fixture.store.edge_count() == 1);
}
