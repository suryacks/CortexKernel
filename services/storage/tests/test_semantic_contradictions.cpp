#include <catch2/catch_test_macros.hpp>
#include "../include/contradiction_detector.hpp"

namespace {

kg::Edge make_edge(const std::string& id, const std::string& subject_id, const std::string& predicate,
                    const std::string& object_id, kg::EdgeClass edge_class) {
    kg::Edge e;
    e.id = id;
    e.subject_id = subject_id;
    e.predicate = predicate;
    e.object_id = object_id;
    e.edge_class = edge_class;
    e.valid_at = "2026-01-01";
    return e;
}

kg::Node make_node(const std::string& id, const std::string& name, kg::NodeType type = kg::NodeType::Concept) {
    kg::Node n;
    n.id = id;
    n.name = name;
    n.type = type;
    return n;
}

}

TEST_CASE("find_semantic_value_behavior_mismatches flags edges whose predicate+object text is nearly identical",
          "[contradiction_detector][semantic]") {
    kg::GraphStore store;
    store.add_node(make_node("health-a", "personal health"));
    store.add_node(make_node("health-b", "personal health"));
    store.add_edge(make_edge("e1", "self", "values", "health-a", kg::EdgeClass::StatedValue));
    store.add_edge(make_edge("e2", "self", "values", "health-b", kg::EdgeClass::BehaviorEvidence));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_semantic_value_behavior_mismatches(0.5f);

    REQUIRE(mismatches.size() == 1);
    REQUIRE(mismatches[0].subject_id == "self");
}

TEST_CASE("find_semantic_value_behavior_mismatches does not flag unrelated text", "[contradiction_detector][semantic]") {
    kg::GraphStore store;
    store.add_node(make_node("cats", "cats"));
    store.add_node(make_node("vacations", "family vacations"));
    store.add_edge(make_edge("e1", "self", "loves", "cats", kg::EdgeClass::StatedValue));
    store.add_edge(make_edge("e2", "self", "avoids", "vacations", kg::EdgeClass::BehaviorEvidence));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_semantic_value_behavior_mismatches(0.5f);

    REQUIRE(mismatches.empty());
}

TEST_CASE("find_semantic_value_behavior_mismatches skips exact subject+predicate+object duplicates",
          "[contradiction_detector][semantic]") {
    kg::GraphStore store;
    store.add_node(make_node("health-a", "personal health"));
    store.add_edge(make_edge("e1", "self", "values", "health-a", kg::EdgeClass::StatedValue));
    store.add_edge(make_edge("e2", "self", "values", "health-a", kg::EdgeClass::BehaviorEvidence));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_semantic_value_behavior_mismatches(0.5f);

    REQUIRE(mismatches.empty());
}

TEST_CASE("find_semantic_value_behavior_mismatches respects the similarity threshold", "[contradiction_detector][semantic]") {
    kg::GraphStore store;
    store.add_node(make_node("health-a", "personal health"));
    store.add_node(make_node("health-b", "personal health"));
    store.add_edge(make_edge("e1", "self", "values", "health-a", kg::EdgeClass::StatedValue));
    store.add_edge(make_edge("e2", "self", "values", "health-b", kg::EdgeClass::BehaviorEvidence));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_semantic_value_behavior_mismatches(1.1f);

    REQUIRE(mismatches.empty());
}

TEST_CASE("find_semantic_value_behavior_mismatches ignores subjects with no BehaviorEvidence edges",
          "[contradiction_detector][semantic]") {
    kg::GraphStore store;
    store.add_node(make_node("health-a", "personal health"));
    store.add_edge(make_edge("e1", "self", "values", "health-a", kg::EdgeClass::StatedValue));

    kg::ContradictionDetector detector(store);
    auto mismatches = detector.find_semantic_value_behavior_mismatches(0.0f);

    REQUIRE(mismatches.empty());
}
