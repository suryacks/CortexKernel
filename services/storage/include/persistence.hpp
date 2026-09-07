#pragma once

#include "graph_store.hpp"
#include "graph_types.hpp"
#include <string>

namespace kg {

class WalWriter {
public:
    explicit WalWriter(const std::string& path);

    void record_add_node(const Node& node);
    void record_add_edge(const Edge& edge);
    void record_invalidate_edge(const std::string& edge_id, const std::string& invalid_at_timestamp);

private:
    void append_line(const std::string& line);

    std::string path_;
};

GraphStore load_graph_store_from_wal(const std::string& path);

}
