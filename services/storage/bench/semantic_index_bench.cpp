#include "../include/semantic_index.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

double to_ms(Clock::duration d) {
    return std::chrono::duration<double, std::milli>(d).count();
}

struct LatencyStats {
    double min_ms;
    double p50_ms;
    double p99_ms;
    double max_ms;
    double mean_ms;
};

LatencyStats compute_stats(std::vector<double> samples_ms) {
    std::sort(samples_ms.begin(), samples_ms.end());
    double sum = 0.0;
    for (double v : samples_ms) {
        sum += v;
    }
    size_t n = samples_ms.size();
    size_t p50_idx = n / 2;
    size_t p99_idx = std::min(n - 1, static_cast<size_t>(static_cast<double>(n) * 0.99));
    return LatencyStats{samples_ms.front(), samples_ms[p50_idx], samples_ms[p99_idx],
                         samples_ms.back(), sum / static_cast<double>(n)};
}

kg::SemanticIndex build_index(size_t size) {
    kg::SemanticIndex index;
    for (size_t i = 0; i < size; ++i) {
        std::string text = "synthetic belief number " + std::to_string(i) + " about health and work and values";
        index.add("id" + std::to_string(i), kg::embed_text(text, 64));
    }
    return index;
}

template <typename Fn>
LatencyStats benchmark(size_t iterations, Fn&& fn) {
    std::vector<double> samples;
    samples.reserve(iterations);
    for (size_t i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        fn();
        auto end = Clock::now();
        samples.push_back(to_ms(end - start));
    }
    return compute_stats(std::move(samples));
}

void print_stats(const std::string& label, const LatencyStats& stats) {
    std::cout << std::left << std::setw(35) << label << std::fixed << std::setprecision(4)
              << "min=" << stats.min_ms << "ms  "
              << "p50=" << stats.p50_ms << "ms  "
              << "p99=" << stats.p99_ms << "ms  "
              << "max=" << stats.max_ms << "ms  "
              << "mean=" << stats.mean_ms << "ms" << std::endl;
}

}

int main() {
    std::vector<size_t> sizes{100, 1000, 10000};
    kg::Embedding query = kg::embed_text("i value my health but keep working late instead", 64);

    for (size_t size : sizes) {
        kg::SemanticIndex index = build_index(size);
        std::cout << "=== index size: " << size << " embeddings ===" << std::endl;
        print_stats("most_similar(top_k=5)", benchmark(20, [&]() {
            auto result = index.most_similar(query, 5);
            (void)result;
        }));
        std::cout << std::endl;
    }

    return 0;
}
