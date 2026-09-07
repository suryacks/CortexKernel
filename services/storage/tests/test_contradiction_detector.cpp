#include <catch2/catch_test_macros.hpp>
#include "../include/contradiction_detector.hpp"

namespace {

kg::Edge make_edge(const std::string& id, const std::string& subject_id, const std::string& predicate,
                    const std::string& object_id, kg::EdgeClass edge_class, const std::string& valid_at) {
    kg::Edge e;
    e.id = id;
    e.subject_id = subject_id;
    e.predicate = predicate;
    e.object_id = object_id;
    e.edge_class = edge_class;
    e.valid_at = valid_at;
    return e;
}

}

TEST_CASE("find_direct_contradictions flags same subject+predicate with different live objects", "[contradiction_detector]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "lives_in", "nyc", kg::EdgeClass::Fact, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "lives_in", "la", kg::EdgeClass::Fact, "2026-02-01"));

    kg::ContradictionDetector detector(store);
    auto contradictions = detector.find_direct_contradictions();

    REQUIRE(contradictions.size() == 1);
    REQUIRE(contradictions[0].subject_id == "self");
    REQUIRE(contradictions[0].predicate == "lives_in");
}

TEST_CASE("find_direct_contradictions ignores a single edge", "[contradiction_detector]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "lives_in", "nyc", kg::EdgeClass::Fact, "2026-01-01"));

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.find_direct_contradictions().empty());
}

TEST_CASE("find_value_behavior_mismatches flags StatedValue vs BehaviorEvidence with different objects", "[contradiction_detector]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "prioritizes", "health", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "prioritizes", "work", kg::EdgeClass::BehaviorEvidence, "2026-02-01"));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_value_behavior_mismatches();

    REQUIRE(mismatches.size() == 1);
}

TEST_CASE("find_value_behavior_mismatches ignores two edges of the same class", "[contradiction_detector]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "prioritizes", "health", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "prioritizes", "work", kg::EdgeClass::StatedValue, "2026-02-01"));

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.find_value_behavior_mismatches().empty());
}

TEST_CASE("classify_drift returns Held for a single live edge", "[contradiction_detector][drift]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "believes", "growth-mindset", kg::EdgeClass::StatedValue, "2026-01-01"));

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.classify_drift("self", "believes") == kg::DriftState::Held);
}

TEST_CASE("classify_drift returns Refined when an older edge was cleanly invalidated", "[contradiction_detector][drift]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "believes", "old-belief", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "believes", "new-belief", kg::EdgeClass::StatedValue, "2026-02-01"));
    store.invalidate_edge("e1", "2026-02-01");

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.classify_drift("self", "believes") == kg::DriftState::Refined);
}

TEST_CASE("classify_drift returns Contradicted for two live edges of the same class", "[contradiction_detector][drift]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "believes", "belief-a", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "believes", "belief-b", kg::EdgeClass::StatedValue, "2026-02-01"));

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.classify_drift("self", "believes") == kg::DriftState::Contradicted);
}

TEST_CASE("classify_drift returns Both when live edges span StatedValue and BehaviorEvidence", "[contradiction_detector][drift]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "prioritizes", "health", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.add_edge(make_edge("e2", "self", "prioritizes", "work", kg::EdgeClass::BehaviorEvidence, "2026-02-01"));

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.classify_drift("self", "prioritizes") == kg::DriftState::Both);
}

TEST_CASE("classify_drift returns Superseded when the only edge was invalidated", "[contradiction_detector][drift]") {
    kg::GraphStore store;
    store.add_edge(make_edge("e1", "self", "believes", "old-belief", kg::EdgeClass::StatedValue, "2026-01-01"));
    store.invalidate_edge("e1", "2026-02-01");

    kg::ContradictionDetector detector(store);
    REQUIRE(detector.classify_drift("self", "believes") == kg::DriftState::Superseded);
}

TEST_CASE("classify_drift throws when no matching edges exist", "[contradiction_detector][error-handling]") {
    kg::GraphStore store;
    kg::ContradictionDetector detector(store);
    REQUIRE_THROWS_AS(detector.classify_drift("nobody", "believes"), std::invalid_argument);
}

TEST_CASE("drift_state_to_string covers every state", "[contradiction_detector]") {
    REQUIRE(kg::drift_state_to_string(kg::DriftState::Held) == "HELD");
    REQUIRE(kg::drift_state_to_string(kg::DriftState::Refined) == "REFINED");
    REQUIRE(kg::drift_state_to_string(kg::DriftState::Contradicted) == "CONTRADICTED");
    REQUIRE(kg::drift_state_to_string(kg::DriftState::Both) == "BOTH");
    REQUIRE(kg::drift_state_to_string(kg::DriftState::Superseded) == "SUPERSEDED");
}
