#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <tuple>

namespace kg {

class Metrics {
public:
    void record_request(const std::string& method, const std::string& path, int status, double duration_ms);
    std::string to_prometheus_text() const;

private:
    struct Counter {
        uint64_t count = 0;
        double total_duration_ms = 0.0;
    };

    mutable std::mutex mutex_;
    std::map<std::tuple<std::string, std::string, int>, Counter> counters_;
};

}
