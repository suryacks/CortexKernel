#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace kg {

using Embedding = std::vector<float>;

Embedding embed_text(const std::string& text, size_t dimensions = 64);
float cosine_similarity(const Embedding& a, const Embedding& b);

struct SimilarityResult {
    std::string id;
    float score;
};

class SemanticIndex {
public:
    void add(const std::string& id, const Embedding& embedding);
    void remove(const std::string& id);
    std::vector<SimilarityResult> most_similar(const Embedding& query, size_t top_k) const;
    size_t size() const;

private:
    std::unordered_map<std::string, Embedding> embeddings_;
};

}
