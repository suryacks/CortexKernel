#include "../include/contradiction_detector.hpp"
#include "../include/graph_store.hpp"
#include "../include/graph_types.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
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

kg::GraphStore build_synthetic_store(size_t edge_count, size_t distinct_predicates, std::mt19937& rng) {
    kg::GraphStore store;

    kg::Edge seed;
    seed.id = "seed";
    seed.subject_id = "self";
    seed.predicate = "p0";
    seed.object_id = "seed-object";
    seed.edge_class = kg::EdgeClass::Fact;
    seed.valid_at = "2026-01-01";
    store.add_edge(seed);

    std::uniform_int_distribution<int> predicate_dist(0, static_cast<int>(distinct_predicates) - 1);
    std::uniform_int_distribution<int> object_dist(0, 9);
    std::uniform_int_distribution<int> class_dist(0, 1);

    for (size_t i = 0; i < edge_count; ++i) {
        kg::Edge e;
        e.id = "e" + std::to_string(i);
        e.subject_id = "self";
        e.predicate = "p" + std::to_string(predicate_dist(rng));
        e.object_id = "o" + std::to_string(object_dist(rng));
        e.edge_class = class_dist(rng) == 0 ? kg::EdgeClass::StatedValue : kg::EdgeClass::BehaviorEvidence;
        e.valid_at = "2026-01-01";
        store.add_edge(e);
    }
    return store;
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
    std::cout << std::left << std::setw(45) << label << std::fixed << std::setprecision(4)
              << "min=" << stats.min_ms << "ms  "
              << "p50=" << stats.p50_ms << "ms  "
              << "p99=" << stats.p99_ms << "ms  "
              << "max=" << stats.max_ms << "ms  "
              << "mean=" << stats.mean_ms << "ms" << std::endl;
}

}

int main() {
    std::mt19937 rng(42);
    std::vector<size_t> sizes{100, 1000, 10000};

    for (size_t size : sizes) {
        kg::GraphStore store = build_synthetic_store(size, 20, rng);
        kg::ContradictionDetector detector(store);

        std::cout << "=== graph size: " << size << " edges ===" << std::endl;

        print_stats("find_direct_contradictions()", benchmark(20, [&]() {
            auto result = detector.find_direct_contradictions();
            (void)result;
        }));

        print_stats("find_value_behavior_mismatches()", benchmark(20, [&]() {
            auto result = detector.find_value_behavior_mismatches();
            (void)result;
        }));

        print_stats("classify_drift(\"self\", \"p0\")", benchmark(20, [&]() {
            auto result = detector.classify_drift("self", "p0");
            (void)result;
        }));

        std::cout << std::endl;
    }

    return 0;
}
