#pragma once

#include "graph_types.hpp"
#include "redis_client.hpp"

#include <optional>
#include <string>

namespace kg {

class NodeCache {
public:
    explicit NodeCache(RedisClient& client, int ttl_seconds = 60);

    std::optional<Node> get(const std::string& id);
    void put(const Node& node);
    void invalidate(const std::string& id);

private:
    static std::string cache_key(const std::string& id);

    RedisClient& client_;
    int ttl_seconds_;
};

}
