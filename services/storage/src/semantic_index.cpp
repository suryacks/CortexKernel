#include "../include/semantic_index.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace kg {

Embedding embed_text(const std::string& text, size_t dimensions) {
    Embedding vec(dimensions, 0.0f);
    std::hash<std::string> hasher;
    for (size_t i = 0; i + 2 < text.size(); ++i) {
        std::string trigram = text.substr(i, 3);
        size_t bucket = hasher(trigram) % dimensions;
        vec[bucket] += 1.0f;
    }
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& v : vec) {
            v /= norm;
        }
    }
    return vec;
}

float cosine_similarity(const Embedding& a, const Embedding& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("Embedding dimension mismatch");
    }
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 0.0f;
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

void SemanticIndex::add(const std::string& id, const Embedding& embedding) {
    embeddings_[id] = embedding;
}

void SemanticIndex::remove(const std::string& id) {
    embeddings_.erase(id);
}

size_t SemanticIndex::size() const {
    return embeddings_.size();
}

std::vector<SimilarityResult> SemanticIndex::most_similar(const Embedding& query, size_t top_k) const {
    std::vector<SimilarityResult> scored;
    scored.reserve(embeddings_.size());
    for (const auto& pair : embeddings_) {
        scored.push_back({pair.first, cosine_similarity(query, pair.second)});
    }
    std::sort(scored.begin(), scored.end(), [](const SimilarityResult& a, const SimilarityResult& b) {
        return a.score > b.score;
    });
    if (scored.size() > top_k) {
        scored.resize(top_k);
    }
    return scored;
}

}
