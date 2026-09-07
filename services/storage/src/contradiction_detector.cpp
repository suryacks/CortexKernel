#include "../include/contradiction_detector.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace kg {

std::string drift_state_to_string(DriftState state) {
    switch (state) {
        case DriftState::Held: return "HELD";
        case DriftState::Refined: return "REFINED";
        case DriftState::Contradicted: return "CONTRADICTED";
        case DriftState::Both: return "BOTH";
        case DriftState::Superseded: return "SUPERSEDED";
    }
    return "UNKNOWN";
}

ContradictionDetector::ContradictionDetector(const GraphStore& store) : store_(store) {}

namespace {

std::map<std::pair<std::string, std::string>, std::vector<const Edge*>> group_by_subject_predicate(
    const std::vector<const Edge*>& edges) {
    std::map<std::pair<std::string, std::string>, std::vector<const Edge*>> groups;
    for (const Edge* edge : edges) {
        groups[{edge->subject_id, edge->predicate}].push_back(edge);
    }
    return groups;
}

bool is_value_behavior_pair(EdgeClass a, EdgeClass b) {
    return (a == EdgeClass::StatedValue && b == EdgeClass::BehaviorEvidence) ||
           (a == EdgeClass::BehaviorEvidence && b == EdgeClass::StatedValue);
}

}

std::vector<Contradiction> ContradictionDetector::find_direct_contradictions() const {
    std::vector<Contradiction> result;
    for (const auto& entry : group_by_subject_predicate(store_.live_edges())) {
        const auto& edges = entry.second;
        for (size_t i = 0; i < edges.size(); ++i) {
            for (size_t j = i + 1; j < edges.size(); ++j) {
                if (edges[i]->object_id != edges[j]->object_id &&
                    edges[i]->edge_class == edges[j]->edge_class) {
                    result.push_back({entry.first.first, entry.first.second, edges[i], edges[j]});
                }
            }
        }
    }
    return result;
}

std::vector<Contradiction> ContradictionDetector::find_value_behavior_mismatches() const {
    std::vector<Contradiction> result;
    for (const auto& entry : group_by_subject_predicate(store_.live_edges())) {
        const auto& edges = entry.second;
        for (size_t i = 0; i < edges.size(); ++i) {
            for (size_t j = i + 1; j < edges.size(); ++j) {
                if (edges[i]->object_id != edges[j]->object_id &&
                    is_value_behavior_pair(edges[i]->edge_class, edges[j]->edge_class)) {
                    result.push_back({entry.first.first, entry.first.second, edges[i], edges[j]});
                }
            }
        }
    }
    return result;
}

DriftState ContradictionDetector::classify_drift(const std::string& subject_id, const std::string& predicate) const {
    std::vector<const Edge*> history;
    for (const Edge* edge : store_.all_edges()) {
        if (edge->subject_id == subject_id && edge->predicate == predicate) {
            history.push_back(edge);
        }
    }

    if (history.empty()) {
        throw std::invalid_argument("No edges found for subject_id=" + subject_id + ", predicate=" + predicate);
    }

    std::sort(history.begin(), history.end(), [](const Edge* a, const Edge* b) {
        return a->valid_at < b->valid_at;
    });

    std::vector<const Edge*> live;
    for (const Edge* edge : history) {
        if (!edge->invalid_at.has_value()) {
            live.push_back(edge);
        }
    }

    if (live.empty()) {
        return DriftState::Superseded;
    }
    if (live.size() == 1) {
        return history.size() == 1 ? DriftState::Held : DriftState::Refined;
    }

    std::set<EdgeClass> live_classes;
    for (const Edge* edge : live) {
        live_classes.insert(edge->edge_class);
    }
    bool spans_value_and_behavior = live_classes.count(EdgeClass::StatedValue) > 0 &&
                                     live_classes.count(EdgeClass::BehaviorEvidence) > 0;
    return spans_value_and_behavior ? DriftState::Both : DriftState::Contradicted;
}

}
