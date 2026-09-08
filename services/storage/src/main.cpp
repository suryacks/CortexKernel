#include "../include/contradiction_detector.hpp"
#include "../include/graph_store.hpp"
#include "../include/httplib.h"
#include "../include/json_translation.hpp"
#include "../include/logger.hpp"
#include "../include/metrics.hpp"
#include "../include/node_cache.hpp"
#include "../include/persistence.hpp"
#include "../include/redis_client.hpp"

#ifdef CORTEXKERNEL_GRPC_ENABLED
#include "../include/grpc_server.hpp"
#include <grpcpp/grpcpp.h>
#include <thread>
#endif

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unordered_map>

using json = nlohmann::json;

namespace {

thread_local std::chrono::steady_clock::time_point g_request_start;

std::string env_or(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : fallback;
}

}

int main() {
    const std::string wal_path = "storage.wal";
    kg::GraphStore store = kg::load_graph_store_from_wal(wal_path);
    kg::WalWriter wal(wal_path);
    kg::Metrics metrics;

    std::string redis_host = env_or("REDIS_HOST", "127.0.0.1");
    int redis_port = std::stoi(env_or("REDIS_PORT", "6379"));
    kg::RedisClient redis_client(redis_host, redis_port);
    kg::NodeCache node_cache(redis_client, 60);

#ifdef CORTEXKERNEL_GRPC_ENABLED
    kg::GrpcStorageService grpc_service(store, wal, node_cache);
    const std::string grpc_address = "0.0.0.0:50051";
    grpc::ServerBuilder grpc_builder;
    grpc_builder.AddListeningPort(grpc_address, grpc::InsecureServerCredentials());
    grpc_builder.RegisterService(&grpc_service);
    std::unique_ptr<grpc::Server> grpc_server = grpc_builder.BuildAndStart();
    std::thread grpc_thread([&grpc_server]() { grpc_server->Wait(); });
    grpc_thread.detach();
    kg::log::info("gRPC storage service listening", {{"address", grpc_address}});
#endif

    httplib::Server svr;
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"}});
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");
        res.status = 204;
    });

    kg::log::info("storage service starting", {
        {"wal_path", wal_path},
        {"node_count", std::to_string(store.node_count())},
        {"edge_count", std::to_string(store.edge_count())},
        {"redis_host", redis_host},
        {"redis_port", std::to_string(redis_port)},
        {"redis_reachable", redis_client.ping() ? "true" : "false"}
    });

    svr.set_pre_routing_handler([](const httplib::Request&, httplib::Response&) {
        g_request_start = std::chrono::steady_clock::now();
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.set_logger([&metrics](const httplib::Request& req, const httplib::Response& res) {
        double duration_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - g_request_start).count();
        metrics.record_request(req.method, req.path, res.status, duration_ms);
    });

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    svr.Post("/nodes", [&store, &wal, &node_cache](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            kg::Node n = kg::node_from_json(body);
            store.add_node(n);
            wal.record_add_node(n);
            node_cache.put(n);
            res.status = 201;
            res.set_content(kg::node_to_json(n).dump(), "application/json");
        } catch (const std::exception& e) {
            kg::log::warn("rejected POST /nodes", {{"error", e.what()}});
            res.status = 400;
            json err{{"error", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    svr.Get(R"(/nodes/([^/]+))", [&store, &node_cache](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];

        auto cached = node_cache.get(id);
        if (cached.has_value()) {
            res.set_header("X-Cache", "HIT");
            res.set_content(kg::node_to_json(*cached).dump(), "application/json");
            return;
        }

        const kg::Node* n = store.get_node(id);
        if (n == nullptr) {
            res.status = 404;
            json err{{"error", "Node not found: " + id}};
            res.set_content(err.dump(), "application/json");
            return;
        }
        node_cache.put(*n);
        res.set_header("X-Cache", "MISS");
        res.set_content(kg::node_to_json(*n).dump(), "application/json");
    });

    svr.Get("/nodes", [&store](const httplib::Request&, httplib::Response& res) {
        json result = json::array();
        for (const auto* n : store.all_nodes()) {
            result.push_back(kg::node_to_json(*n));
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/edges", [&store](const httplib::Request&, httplib::Response& res) {
        json result = json::array();
        for (const auto* e : store.all_edges()) {
            result.push_back(kg::edge_to_json(*e));
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Post("/edges", [&store, &wal](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            kg::Edge e = kg::edge_from_json(body);
            store.add_edge(e);
            wal.record_add_edge(e);
            res.status = 201;
            res.set_content(kg::edge_to_json(e).dump(), "application/json");
        } catch (const std::exception& e) {
            kg::log::warn("rejected POST /edges", {{"error", e.what()}});
            res.status = 400;
            json err{{"error", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    svr.Get(R"(/nodes/([^/]+)/edges)", [&store](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        json result = json::array();
        for (const auto* e : store.edges_from(id)) {
            result.push_back(kg::edge_to_json(*e));
        }
        for (const auto* e : store.edges_to(id)) {
            result.push_back(kg::edge_to_json(*e));
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/stats", [&store](const httplib::Request&, httplib::Response& res) {
        json stats{
            {"node_count", store.node_count()},
            {"edge_count", store.edge_count()}
        };
        res.set_content(stats.dump(), "application/json");
    });

    svr.Get("/contradictions", [&store](const httplib::Request&, httplib::Response& res) {
        kg::ContradictionDetector detector(store);
        json result = json::array();
        for (const auto& c : detector.find_direct_contradictions()) {
            result.push_back({
                {"type", "direct"},
                {"subject_id", c.subject_id},
                {"predicate", c.predicate},
                {"edge_a", kg::edge_to_json(*c.edge_a)},
                {"edge_b", kg::edge_to_json(*c.edge_b)}
            });
        }
        for (const auto& c : detector.find_value_behavior_mismatches()) {
            result.push_back({
                {"type", "value_behavior"},
                {"subject_id", c.subject_id},
                {"predicate", c.predicate},
                {"edge_a", kg::edge_to_json(*c.edge_a)},
                {"edge_b", kg::edge_to_json(*c.edge_b)}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get(R"(/drift/([^/]+)/([^/]+))", [&store](const httplib::Request& req, httplib::Response& res) {
        std::string subject_id = req.matches[1];
        std::string predicate = req.matches[2];
        kg::ContradictionDetector detector(store);
        try {
            kg::DriftState state = detector.classify_drift(subject_id, predicate);
            json result{
                {"subject_id", subject_id},
                {"predicate", predicate},
                {"state", kg::drift_state_to_string(state)}
            };
            res.set_content(result.dump(), "application/json");
        } catch (const std::invalid_argument& e) {
            res.status = 404;
            json err{{"error", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    svr.Get("/metrics", [&metrics](const httplib::Request&, httplib::Response& res) {
        res.set_content(metrics.to_prometheus_text(), "text/plain; version=0.0.4");
    });

    kg::log::info("storage service listening", {{"port", "8080"}});
    svr.listen("0.0.0.0", 8080);

    return 0;
}