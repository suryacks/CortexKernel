#include <catch2/catch_test_macros.hpp>
#include "../include/redis_client.hpp"

TEST_CASE("RedisClient reports failure gracefully when redis is unreachable", "[redis_client]") {
    kg::RedisClient client("127.0.0.1", 1);
    REQUIRE_FALSE(client.ping());
    REQUIRE_FALSE(client.set("k", "v"));
    REQUIRE_FALSE(client.get("k").has_value());
}

TEST_CASE("RedisClient can ping a local redis-server", "[redis_client][integration]") {
    kg::RedisClient client("127.0.0.1", 6379);
    if (!client.ping()) {
        SKIP("no local redis-server reachable on 127.0.0.1:6379");
    }
    REQUIRE(client.ping());
}

TEST_CASE("RedisClient set/get/del round-trip against a local redis-server", "[redis_client][integration]") {
    kg::RedisClient client("127.0.0.1", 6379);
    if (!client.ping()) {
        SKIP("no local redis-server reachable on 127.0.0.1:6379");
    }

    REQUIRE(client.set("cortexkernel:test:key", "hello"));
    auto value = client.get("cortexkernel:test:key");
    REQUIRE(value.has_value());
    REQUIRE(*value == "hello");

    REQUIRE(client.del("cortexkernel:test:key"));
    REQUIRE_FALSE(client.get("cortexkernel:test:key").has_value());
}

TEST_CASE("RedisClient get on a missing key returns nullopt", "[redis_client][integration]") {
    kg::RedisClient client("127.0.0.1", 6379);
    if (!client.ping()) {
        SKIP("no local redis-server reachable on 127.0.0.1:6379");
    }
    REQUIRE_FALSE(client.get("cortexkernel:test:definitely-not-set").has_value());
}
