#include "../include/node_cache.hpp"
#include "../include/json_translation.hpp"

namespace kg {

NodeCache::NodeCache(RedisClient& client, int ttl_seconds) : client_(client), ttl_seconds_(ttl_seconds) {}

std::string NodeCache::cache_key(const std::string& id) {
    return "cortexkernel:node:" + id;
}

std::optional<Node> NodeCache::get(const std::string& id) {
    auto cached = client_.get(cache_key(id));
    if (!cached.has_value()) {
        return std::nullopt;
    }
    try {
        return node_from_json(json::parse(*cached));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

void NodeCache::put(const Node& node) {
    client_.set(cache_key(node.id), node_to_json(node).dump(), ttl_seconds_);
}

void NodeCache::invalidate(const std::string& id) {
    client_.del(cache_key(id));
}

}
