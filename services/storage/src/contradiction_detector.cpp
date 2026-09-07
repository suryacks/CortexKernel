#include "../include/contradiction_detector.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>

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

std::unordered_map<std::string, const Edge*> first_edge_per_object(const std::vector<const Edge*>& edges) {
    std::unordered_map<std::string, const Edge*> result;
    for (const Edge* edge : edges) {
        result.emplace(edge->object_id, edge);
    }
    return result;
}

}

std::vector<Contradiction> ContradictionDetector::find_direct_contradictions() const {
    std::vector<Contradiction> result;
    for (const auto& entry : group_by_subject_predicate(store_.live_edges())) {
        std::unordered_map<EdgeClass, std::vector<const Edge*>> by_class;
        for (const Edge* edge : entry.second) {
            by_class[edge->edge_class].push_back(edge);
        }
        for (const auto& class_entry : by_class) {
            auto reps = first_edge_per_object(class_entry.second);
            if (reps.size() < 2) {
                continue;
            }
            auto it = reps.begin();
            const Edge* anchor = it->second;
            for (++it; it != reps.end(); ++it) {
                result.push_back({entry.first.first, entry.first.second, anchor, it->second});
            }
        }
    }
    return result;
}

std::vector<Contradiction> ContradictionDetector::find_value_behavior_mismatches() const {
    std::vector<Contradiction> result;
    for (const auto& entry : group_by_subject_predicate(store_.live_edges())) {
        std::vector<const Edge*> stated;
        std::vector<const Edge*> behavior;
        for (const Edge* edge : entry.second) {
            if (edge->edge_class == EdgeClass::StatedValue) {
                stated.push_back(edge);
            } else if (edge->edge_class == EdgeClass::BehaviorEvidence) {
                behavior.push_back(edge);
            }
        }
        if (stated.empty() || behavior.empty()) {
            continue;
        }
        auto stated_reps = first_edge_per_object(stated);
        auto behavior_reps = first_edge_per_object(behavior);
        for (const auto& stated_rep : stated_reps) {
            for (const auto& behavior_rep : behavior_reps) {
                if (stated_rep.first != behavior_rep.first) {
                    result.push_back({entry.first.first, entry.first.second, stated_rep.second, behavior_rep.second});
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
