#include "../include/graph_store.hpp"

namespace kg {

void GraphStore::add_node(const Node& node) {
    nodes_[node.id] = node;
}

void GraphStore::add_edge(const Edge& edge) {
    edges_[edge.id] = edge;
}

const Node* GraphStore::get_node(const std::string& id) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        return nullptr;
    }
    return &(it->second);
}

std::vector<const Node*> GraphStore::all_nodes() const {
    std::vector<const Node*> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(&(pair.second));
    }
    return result;
}

std::vector<const Edge*> GraphStore::edges_from(const std::string& subject_id) const {
    std::vector<const Edge*> result;
    for (const auto& pair : edges_) {
        if (pair.second.subject_id == subject_id) {
            result.push_back(&(pair.second));
        }
    }
    return result;
}

std::vector<const Edge*> GraphStore::edges_to(const std::string& object_id) const {
    std::vector<const Edge*> result;
    for (const auto& pair : edges_) {
        if (pair.second.object_id == object_id) {
            result.push_back(&(pair.second));
        }
    }
    return result;
}

std::vector<const Edge*> GraphStore::live_edges() const {
    std::vector<const Edge*> result;
    for (const auto& pair : edges_) {
        if (!pair.second.invalid_at.has_value()) {
            result.push_back(&(pair.second));
        }
    }
    return result;
}

std::vector<const Edge*> GraphStore::all_edges() const {
    std::vector<const Edge*> result;
    result.reserve(edges_.size());
    for (const auto& pair : edges_) {
        result.push_back(&(pair.second));
    }
    return result;
}

bool GraphStore::invalidate_edge(const std::string& edge_id, const std::string& invalid_at_timestamp) {
    auto it = edges_.find(edge_id);
    if (it == edges_.end()) {
        return false;
    }
    it->second.invalid_at = invalid_at_timestamp;
    return true;
}

size_t GraphStore::node_count() const {
    return nodes_.size();
}

size_t GraphStore::edge_count() const {
    return edges_.size();
}

} // namespace kg