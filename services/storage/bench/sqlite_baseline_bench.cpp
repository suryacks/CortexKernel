#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
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

void print_stats(const std::string& label, const LatencyStats& stats) {
    std::cout << std::left << std::setw(35) << label << std::fixed << std::setprecision(4)
              << "min=" << stats.min_ms << "ms  "
              << "p50=" << stats.p50_ms << "ms  "
              << "p99=" << stats.p99_ms << "ms  "
              << "max=" << stats.max_ms << "ms  "
              << "mean=" << stats.mean_ms << "ms" << std::endl;
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

void exec_or_throw(sqlite3* db, const std::string& sql) {
    char* error_message = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_message) != SQLITE_OK) {
        std::string message = error_message ? error_message : "unknown sqlite error";
        sqlite3_free(error_message);
        throw std::runtime_error(message);
    }
}

sqlite3* build_synthetic_db(size_t edge_count, size_t distinct_predicates, std::mt19937& rng) {
    sqlite3* db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        throw std::runtime_error("failed to open in-memory sqlite database");
    }

    exec_or_throw(db,
        "CREATE TABLE edges ("
        "id TEXT, subject_id TEXT, predicate TEXT, object_id TEXT, "
        "edge_class TEXT, invalid_at TEXT)");

    exec_or_throw(db, "BEGIN TRANSACTION");
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT INTO edges (id, subject_id, predicate, object_id, edge_class, invalid_at) "
        "VALUES (?, 'self', ?, ?, ?, NULL)",
        -1, &stmt, nullptr);

    exec_or_throw(db,
        "INSERT INTO edges (id, subject_id, predicate, object_id, edge_class, invalid_at) "
        "VALUES ('seed', 'self', 'p0', 'seed-object', 'Fact', NULL)");

    std::uniform_int_distribution<int> predicate_dist(0, static_cast<int>(distinct_predicates) - 1);
    std::uniform_int_distribution<int> object_dist(0, 9);
    std::uniform_int_distribution<int> class_dist(0, 1);

    for (size_t i = 0; i < edge_count; ++i) {
        std::string id = "e" + std::to_string(i);
        std::string predicate = "p" + std::to_string(predicate_dist(rng));
        std::string object_id = "o" + std::to_string(object_dist(rng));
        std::string edge_class = class_dist(rng) == 0 ? "StatedValue" : "BehaviorEvidence";

        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, predicate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, object_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, edge_class.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    exec_or_throw(db, "COMMIT");

    return db;
}

size_t naive_direct_contradiction_count(sqlite3* db) {
    const char* query =
        "SELECT COUNT(*) FROM edges a JOIN edges b "
        "ON a.subject_id = b.subject_id "
        "AND a.predicate = b.predicate "
        "AND a.edge_class = b.edge_class "
        "AND a.object_id <> b.object_id "
        "AND a.invalid_at IS NULL AND b.invalid_at IS NULL "
        "AND a.id < b.id";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    size_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return count;
}

}

int main() {
    std::mt19937 rng(42);
    std::vector<size_t> sizes{100, 1000, 10000};

    for (size_t size : sizes) {
        sqlite3* db = build_synthetic_db(size, 20, rng);

        std::cout << "=== graph size: " << size << " edges (naive SQLite self-join) ===" << std::endl;
        print_stats("naive_direct_contradiction_count()", benchmark(20, [&]() {
            auto result = naive_direct_contradiction_count(db);
            (void)result;
        }));
        std::cout << std::endl;

        sqlite3_close(db);
    }

    return 0;
}
