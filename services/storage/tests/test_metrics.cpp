#include <catch2/catch_test_macros.hpp>
#include "../include/metrics.hpp"

TEST_CASE("Metrics records request counts and durations", "[metrics]") {
    kg::Metrics metrics;
    metrics.record_request("GET", "/health", 200, 1.5);
    metrics.record_request("GET", "/health", 200, 2.5);

    std::string text = metrics.to_prometheus_text();
    REQUIRE(text.find("cortexkernel_http_requests_total{method=\"GET\",path=\"/health\",status=\"200\"} 2") != std::string::npos);
    REQUIRE(text.find("cortexkernel_http_request_duration_ms_sum{method=\"GET\",path=\"/health\",status=\"200\"} 4") != std::string::npos);
}

TEST_CASE("Metrics tracks distinct routes and statuses separately", "[metrics]") {
    kg::Metrics metrics;
    metrics.record_request("GET", "/nodes/n1", 200, 1.0);
    metrics.record_request("GET", "/nodes/n1", 404, 1.0);
    metrics.record_request("POST", "/nodes", 201, 1.0);

    std::string text = metrics.to_prometheus_text();
    REQUIRE(text.find("path=\"/nodes/n1\",status=\"200\"} 1") != std::string::npos);
    REQUIRE(text.find("path=\"/nodes/n1\",status=\"404\"} 1") != std::string::npos);
    REQUIRE(text.find("path=\"/nodes\",status=\"201\"} 1") != std::string::npos);
}

TEST_CASE("Metrics with no requests still produces valid header text", "[metrics]") {
    kg::Metrics metrics;
    std::string text = metrics.to_prometheus_text();
    REQUIRE(text.find("# TYPE cortexkernel_http_requests_total counter") != std::string::npos);
}
