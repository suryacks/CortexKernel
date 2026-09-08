#pragma once

#include "graph_types.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace kg {

class GraphStore {
public:
    void add_node(const Node& node);
    void add_edge(const Edge& edge);

    const Node* get_node(const std::string& id) const;
    std::vector<const Node*> all_nodes() const;

    std::vector<const Edge*> edges_from(const std::string& subject_id) const;
    std::vector<const Edge*> edges_to(const std::string& object_id) const;
    std::vector<const Edge*> live_edges() const;
    std::vector<const Edge*> all_edges() const;

    bool invalidate_edge(const std::string& edge_id, const std::string& invalid_at_timestamp);

    size_t node_count() const;
    size_t edge_count() const;

private:
    std::unordered_map<std::string, Node> nodes_;
    std::unordered_map<std::string, Edge> edges_;
};

} // namespace kg