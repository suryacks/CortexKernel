#include <catch2/catch_test_macros.hpp>
#include "../include/graph_store.hpp"

TEST_CASE("GraphStore stores and retrieves nodes", "[graph_store]") {
    kg::GraphStore store;

    kg::Node n;
    n.id = "n1";
    n.type = kg::NodeType::Concept;
    n.name = "test concept";
    n.layer = "life";
    store.add_node(n);

    REQUIRE(store.node_count() == 1);

    const kg::Node* found = store.get_node("n1");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "test concept");

    const kg::Node* missing = store.get_node("does-not-exist");
    REQUIRE(missing == nullptr);
}

TEST_CASE("GraphStore traverses edges by subject and object", "[graph_store]") {
    kg::GraphStore store;

    kg::Edge e;
    e.id = "e1";
    e.subject_id = "n1";
    e.object_id = "n2";
    e.predicate = "relates_to";
    e.edge_class = kg::EdgeClass::Fact;
    e.confidence = 0.9;
    store.add_edge(e);

    auto from_n1 = store.edges_from("n1");
    REQUIRE(from_n1.size() == 1);
    REQUIRE(from_n1[0]->predicate == "relates_to");

    auto to_n2 = store.edges_to("n2");
    REQUIRE(to_n2.size() == 1);

    auto from_nonexistent = store.edges_from("n99");
    REQUIRE(from_nonexistent.size() == 0);
}

TEST_CASE("GraphStore invalidates edges without deleting them", "[graph_store][bi-temporal]") {
    kg::GraphStore store;

    kg::Edge e;
    e.id = "e1";
    e.subject_id = "n1";
    e.object_id = "n2";
    e.predicate = "believes";
    e.edge_class = kg::EdgeClass::StatedValue;
    store.add_edge(e);

    REQUIRE(store.live_edges().size() == 1);

    bool ok = store.invalidate_edge("e1", "2026-06-01");
    REQUIRE(ok == true);

    // Still exists, just no longer "live" — this is the core §5.2
    // invalidate-don't-delete behavior, worth its own explicit test.
    REQUIRE(store.edge_count() == 1);
    REQUIRE(store.live_edges().size() == 0);

    bool ok_missing = store.invalidate_edge("does-not-exist", "2026-01-01");
    REQUIRE(ok_missing == false);
}