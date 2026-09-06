#pragma once 

#include "graph_types.hpp"
#include "json.hpp"

namespace kg {
    using json = nlohmann::json;

    std::string node_type_to_string(NodeType t);
    NodeType node_type_from_string(const std::string& s);

    std::string edge_class_to_string(EdgeClass c);
    EdgeClass edge_class_from_string(const std::string& s);

    json node_to_json(const Node& n);
    Node node_from_json(const json& j);

    json edge_to_json(const Edge& e);
    Edge edge_from_json(const json& j);
}
