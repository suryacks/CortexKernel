#include "../include/graph_store.hpp"
#include "../include/httplib.h"
#include "../include/json_translation.hpp"
#include "../include/logger.hpp"
#include "../include/persistence.hpp"

#include <iostream>
#include <sstream>
#include <unordered_map>

using json = nlohmann::json;


int main() {
    const std::string wal_path = "storage.wal";
    kg::GraphStore store = kg::load_graph_store_from_wal(wal_path);
    kg::WalWriter wal(wal_path);
    httplib::Server svr;

    kg::log::info("storage service starting", {
        {"wal_path", wal_path},
        {"node_count", std::to_string(store.node_count())},
        {"edge_count", std::to_string(store.edge_count())}
    });

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    svr.Post("/nodes", [&store, &wal](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            kg::Node n = kg::node_from_json(body);
            store.add_node(n);
            wal.record_add_node(n);
            res.status = 201;
            res.set_content(kg::node_to_json(n).dump(), "application/json");
        } catch (const std::exception& e) {
            kg::log::warn("rejected POST /nodes", {{"error", e.what()}});
            res.status = 400;
            json err{{"error", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    svr.Get(R"(/nodes/([^/]+))", [&store](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        const kg::Node* n = store.get_node(id);
        if (n == nullptr) {
            res.status = 404;
            json err{{"error", "Node not found: " + id}};
            res.set_content(err.dump(), "application/json");
            return;
        }
        res.set_content(kg::node_to_json(*n).dump(), "application/json");
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

    kg::log::info("storage service listening", {{"port", "8080"}});
    svr.listen("0.0.0.0", 8080);

    return 0;
}