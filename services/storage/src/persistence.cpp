#include "../include/persistence.hpp"
#include "../include/json_translation.hpp"

#include <fstream>
#include <stdexcept>

namespace kg {

WalWriter::WalWriter(const std::string& path) : path_(path) {}

void WalWriter::append_line(const std::string& line) {
    std::ofstream out(path_, std::ios::app);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open WAL file for append: " + path_);
    }
    out << line << '\n';
}

void WalWriter::record_add_node(const Node& node) {
    json entry{{"op", "add_node"}, {"node", node_to_json(node)}};
    append_line(entry.dump());
}

void WalWriter::record_add_edge(const Edge& edge) {
    json entry{{"op", "add_edge"}, {"edge", edge_to_json(edge)}};
    append_line(entry.dump());
}

void WalWriter::record_invalidate_edge(const std::string& edge_id, const std::string& invalid_at_timestamp) {
    json entry{{"op", "invalidate_edge"}, {"edge_id", edge_id}, {"invalid_at", invalid_at_timestamp}};
    append_line(entry.dump());
}

GraphStore load_graph_store_from_wal(const std::string& path) {
    GraphStore store;
    std::ifstream in(path);
    if (!in.is_open()) {
        return store;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        json entry = json::parse(line);
        std::string op = entry.at("op").get<std::string>();
        if (op == "add_node") {
            store.add_node(node_from_json(entry.at("node")));
        } else if (op == "add_edge") {
            store.add_edge(edge_from_json(entry.at("edge")));
        } else if (op == "invalidate_edge") {
            store.invalidate_edge(entry.at("edge_id").get<std::string>(), entry.at("invalid_at").get<std::string>());
        } else {
            throw std::runtime_error("Unknown WAL op: " + op);
        }
    }
    return store;
}

}
