#include <catch2/catch_test_macros.hpp>
#include "../include/json_translation.hpp"

TEST_CASE("Node round-trips through JSON correctly", "[json_translation]") {
    kg::Node n;
    n.id = "n1";
    n.type = kg::NodeType::Person;
    n.name = "example";
    n.layer = "life";

    kg::json j = kg::node_to_json(n);
    REQUIRE(j["id"] == "n1");
    REQUIRE(j["type"] == "Person");
    REQUIRE(j["name"] == "example");

    kg::Node roundtrip = kg::node_from_json(j);
    REQUIRE(roundtrip.id == n.id);
    REQUIRE(roundtrip.type == n.type);
    REQUIRE(roundtrip.name == n.name);
}

TEST_CASE("node_from_json throws on unknown NodeType", "[json_translation][error-handling]") {
    kg::json bad = kg::json{{"id", "n1"}, {"type", "NotReal"}, {"name", "x"}};
    REQUIRE_THROWS_AS(kg::node_from_json(bad), std::invalid_argument);
}

TEST_CASE("node_from_json throws on missing required field", "[json_translation][error-handling]") {
    kg::json missing_name = kg::json{{"id", "n1"}, {"type", "Person"}};
    REQUIRE_THROWS(kg::node_from_json(missing_name));
}

TEST_CASE("Edge round-trips through JSON, including null invalid_at", "[json_translation]") {
    kg::Edge e;
    e.id = "e1";
    e.subject_id = "n1";
    e.object_id = "n2";
    e.predicate = "wants";
    e.edge_class = kg::EdgeClass::StatedValue;
    e.confidence = 0.95;

    kg::json j = kg::edge_to_json(e);
    REQUIRE(j["from"] == "n1");
    REQUIRE(j["to"] == "n2");
    REQUIRE(j["edge_class"] == "StatedValue");
    REQUIRE(j["invalid_at"].is_null());

    kg::Edge roundtrip = kg::edge_from_json(j);
    REQUIRE(roundtrip.subject_id == e.subject_id);
    REQUIRE(roundtrip.object_id == e.object_id);
    REQUIRE_FALSE(roundtrip.invalid_at.has_value());
}

TEST_CASE("Edge with invalid_at set round-trips correctly", "[json_translation][bi-temporal]") {
    kg::Edge e;
    e.id = "e1";
    e.subject_id = "n1";
    e.object_id = "n2";
    e.predicate = "wants";
    e.edge_class = kg::EdgeClass::StatedValue;
    e.invalid_at = "2026-06-01";

    kg::json j = kg::edge_to_json(e);
    REQUIRE(j["invalid_at"] == "2026-06-01");

    kg::Edge roundtrip = kg::edge_from_json(j);
    REQUIRE(roundtrip.invalid_at.has_value());
    REQUIRE(roundtrip.invalid_at.value() == "2026-06-01");
}