#include "../include/json_translation.hpp"
#include <stdexcept>

namespace kg {

std::string node_type_to_string(NodeType t) {
    switch (t) {
        case NodeType::Self: return "Self";
        case NodeType::Person: return "Person";
        case NodeType::Place: return "Place";
        case NodeType::Org: return "Org";
        case NodeType::Project: return "Project";
        case NodeType::Concept: return "Concept";
        case NodeType::Value: return "Value";
        case NodeType::Event:    return "Event";
        case NodeType::Emotion:  return "Emotion";
        case NodeType::Artifact: return "Artifact";
        case NodeType::Task:     return "Task";
    }
    return "Unknown";
}
NodeType node_type_from_string(const std::string& s) {
    if (s == "Self") return NodeType::Self;
    if (s == "Person") return NodeType::Person;
    if (s == "Place") return NodeType::Place;
    if (s == "Org") return NodeType::Org;
    if (s == "Project") return NodeType::Project;
    if (s == "Concept") return NodeType::Concept;
    if (s == "Value") return NodeType::Value;
    if (s == "Event") return NodeType::Event;
    if (s == "Emotion") return NodeType::Emotion;
    if (s == "Artifact") return NodeType::Artifact;
    if (s == "Task") return NodeType::Task;
    throw std::invalid_argument("Unknown node type");
}
std::string edge_class_to_string(EdgeClass c) {
    switch (c) {
        case EdgeClass::StatedValue: return "StatedValue";
        case EdgeClass::BehaviorEvidence: return "BehaviorEvidence";
        case EdgeClass::Fact: return "Fact";
        case EdgeClass::Emotion: return "Emotion";
        case EdgeClass::Reflection: return "Reflection";
    }
    return "Unknown";
}
EdgeClass edge_class_from_string(const std::string& s) {
    if (s == "StatedValue") return EdgeClass::StatedValue;
    if (s == "BehaviorEvidence") return EdgeClass::BehaviorEvidence;
    if (s == "Fact") return EdgeClass::Fact;
    if (s == "Emotion") return EdgeClass::Emotion;
    if (s == "Reflection") return EdgeClass::Reflection;
    throw std::invalid_argument("Unknown EdgeClass string: " + s);
}

json node_to_json(const Node& n) {
    return json{
        {"id", n.id},
        {"type", node_type_to_string(n.type)},
        {"name", n.name},
        {"layer", n.layer},
        {"source_ref", n.source_ref},
        {"created_at", n.created_at},
        {"updated_at", n.updated_at}
    };
}
Node node_from_json(const json& j) {
    Node n;
    n.id = j.at("id").get<std::string>();
    n.type = node_type_from_string(j.at("type").get<std::string>());
    n.name = j.at("name").get<std::string>();
    n.layer = j.value("layer", "life");
    n.source_ref = j.value("source_ref", "");
    n.created_at = j.value("created_at", "");
    n.updated_at = j.value("updated_at", "");
    return n;
}
json edge_to_json(const Edge& e) {
    json j{
        {"id", e.id},
        {"from", e.subject_id},
        {"predicate", e.predicate},
        {"to", e.object_id},
        {"edge_class", edge_class_to_string(e.edge_class)},
        {"valid_at", e.valid_at},
        {"recorded_at", e.recorded_at},
        {"confidence", e.confidence},
        {"evidence_span", e.evidence_span},
        {"source_ref", e.source_ref},
        {"layer", e.layer},
        {"extractor", e.extractor}
    };
    if (e.invalid_at.has_value()) {
        j["invalid_at"] = e.invalid_at.value();
    } else {
        j["invalid_at"] = nullptr;
    }
    return j;
}
Edge edge_from_json(const json& j) {
    Edge e;
    e.id = j.at("id").get<std::string>();
    e.subject_id = j.at("from").get<std::string>();
    e.predicate = j.at("predicate").get<std::string>();
    e.object_id = j.at("to").get<std::string>();
    e.edge_class = edge_class_from_string(j.at("edge_class").get<std::string>());
    e.valid_at = j.value("valid_at", "");
    e.recorded_at = j.value("recorded_at", "");
    if (j.contains("invalid_at") && !j["invalid_at"].is_null()) {
        e.invalid_at = j["invalid_at"].get<std::string>();
    }
    e.confidence = j.value("confidence", 1.0);
    e.evidence_span = j.value("evidence_span", "");
    e.source_ref = j.value("source_ref", "");
    e.layer = j.value("layer", "life");
    e.extractor = j.value("extractor", "manual");
    return e;
}
}