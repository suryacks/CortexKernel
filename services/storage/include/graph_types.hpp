#pragma once 
#include <string> 
#include <optional> 

namespace kg {
    enum class NodeType {
        Self,
        Person,
        Place,
        Org,
        Project,
        Concept,
        Value,
        Event,
        Emotion,
        Artifact,
        Task
    }; 
    enum class EdgeClass { 
        StatedValue,
        BehaviorEvidence,
        Fact,
        Emotion,
        Reflection
    };
    struct Node {
        std::string id; 
        NodeType type; 
        std::string name; 
        std::string layer; 
        std::string source_ref; 
        std::string created_at;
        std::string updated_at;
    };
    struct Edge {
        std::string id;
        std::string subject_id;
        std::string predicate;
        std::string object_id;
        EdgeClass edge_class;
        std::string valid_at;
        std::string recorded_at;
        std::optional<std::string> invalid_at;
        double confidence;
        std::string evidence_span;
        std::string source_ref;
        std::string layer;
        std::string extractor;
    };
}