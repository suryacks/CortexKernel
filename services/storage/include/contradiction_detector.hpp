#pragma once

#include "graph_store.hpp"
#include "graph_types.hpp"
#include <string>
#include <vector>

namespace kg {

enum class DriftState {
    Held,
    Refined,
    Contradicted,
    Both,
    Superseded
};

std::string drift_state_to_string(DriftState state);

struct Contradiction {
    std::string subject_id;
    std::string predicate;
    const Edge* edge_a;
    const Edge* edge_b;
};

class ContradictionDetector {
public:
    explicit ContradictionDetector(const GraphStore& store);

    std::vector<Contradiction> find_direct_contradictions() const;
    std::vector<Contradiction> find_value_behavior_mismatches() const;
    std::vector<Contradiction> find_semantic_value_behavior_mismatches(float similarity_threshold = 0.5f) const;
    DriftState classify_drift(const std::string& subject_id, const std::string& predicate) const;

private:
    const GraphStore& store_;
};

}
