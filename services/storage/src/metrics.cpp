#include "../include/metrics.hpp"

#include <sstream>

namespace kg {

void Metrics::record_request(const std::string& method, const std::string& path, int status, double duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& counter = counters_[std::make_tuple(method, path, status)];
    counter.count += 1;
    counter.total_duration_ms += duration_ms;
}

std::string Metrics::to_prometheus_text() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;

    out << "# HELP cortexkernel_http_requests_total Total HTTP requests handled\n";
    out << "# TYPE cortexkernel_http_requests_total counter\n";
    for (const auto& entry : counters_) {
        const auto& [method, path, status] = entry.first;
        out << "cortexkernel_http_requests_total{method=\"" << method << "\",path=\"" << path
            << "\",status=\"" << status << "\"} " << entry.second.count << "\n";
    }

    out << "# HELP cortexkernel_http_request_duration_ms_sum Sum of request durations in milliseconds\n";
    out << "# TYPE cortexkernel_http_request_duration_ms_sum counter\n";
    for (const auto& entry : counters_) {
        const auto& [method, path, status] = entry.first;
        out << "cortexkernel_http_request_duration_ms_sum{method=\"" << method << "\",path=\"" << path
            << "\",status=\"" << status << "\"} " << entry.second.total_duration_ms << "\n";
    }

    return out.str();
}

}
