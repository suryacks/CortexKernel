#include "../include/graph_store.hpp"
#include "../include/httplib.h"
#include "../include/json_translation.hpp"

#include <iostream>
#include <sstream>
#include <unordered_map>

using json = nlohmann::json;


int main() {
    kg::GraphStore store; 
    httplib::Server svr; 
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    svr.Post("/nodes", [&store](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            kg::Node n = kg::node_from_json(body);
            store.add_node(n);
            res.status = 201;
            res.set_content(kg::node_to_json(n).dump(), "application/json");
        } catch (const std::exception& e) {
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

    svr.Post("/edges", [&store](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            kg::Edge e = kg::edge_from_json(body);
            store.add_edge(e);
            res.status = 201;
            res.set_content(kg::edge_to_json(e).dump(), "application/json");
        } catch (const std::exception& e) {
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

    std::cout << "CortexKernel storage service listening on port 8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}