#include <catch2/catch_test_macros.hpp>
#include "../include/semantic_index.hpp"

#include <cmath>

TEST_CASE("cosine_similarity returns 1 for identical vectors", "[semantic_index]") {
    kg::Embedding a{1.0f, 0.0f, 0.0f};
    REQUIRE(std::abs(kg::cosine_similarity(a, a) - 1.0f) < 1e-5f);
}

TEST_CASE("cosine_similarity returns 0 for orthogonal vectors", "[semantic_index]") {
    kg::Embedding a{1.0f, 0.0f};
    kg::Embedding b{0.0f, 1.0f};
    REQUIRE(std::abs(kg::cosine_similarity(a, b)) < 1e-5f);
}

TEST_CASE("cosine_similarity throws on dimension mismatch", "[semantic_index][error-handling]") {
    kg::Embedding a{1.0f, 0.0f};
    kg::Embedding b{1.0f, 0.0f, 0.0f};
    REQUIRE_THROWS_AS(kg::cosine_similarity(a, b), std::invalid_argument);
}

TEST_CASE("SemanticIndex returns most similar entries in descending order", "[semantic_index]") {
    kg::SemanticIndex index;
    index.add("close", kg::Embedding{1.0f, 0.0f});
    index.add("far", kg::Embedding{0.0f, 1.0f});
    index.add("exact", kg::Embedding{0.9f, 0.1f});

    auto results = index.most_similar(kg::Embedding{1.0f, 0.0f}, 2);

    REQUIRE(results.size() == 2);
    REQUIRE(results[0].id == "close");
    REQUIRE(results[1].id == "exact");
}

TEST_CASE("SemanticIndex remove drops an entry from future queries", "[semantic_index]") {
    kg::SemanticIndex index;
    index.add("a", kg::Embedding{1.0f, 0.0f});
    REQUIRE(index.size() == 1);
    index.remove("a");
    REQUIRE(index.size() == 0);
}

TEST_CASE("embed_text produces a unit-length vector of the requested dimension", "[semantic_index]") {
    kg::Embedding e = kg::embed_text("exercises every single day", 32);
    REQUIRE(e.size() == 32);

    float norm = 0.0f;
    for (float v : e) {
        norm += v * v;
    }
    REQUIRE(std::abs(std::sqrt(norm) - 1.0f) < 1e-4f);
}

TEST_CASE("embed_text returns a zero vector for input too short to form a trigram", "[semantic_index][error-handling]") {
    kg::Embedding e = kg::embed_text("", 16);
    for (float v : e) {
        REQUIRE(v == 0.0f);
    }
}
