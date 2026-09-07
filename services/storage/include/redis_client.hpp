#pragma once

#include <optional>
#include <string>
#include <vector>

namespace kg {

class RedisClient {
public:
    RedisClient(std::string host, int port);
    ~RedisClient();

    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    bool ping();
    bool set(const std::string& key, const std::string& value, int ttl_seconds = 0);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);

private:
    bool ensure_connected();
    void disconnect();
    bool send_command(const std::vector<std::string>& args);
    std::optional<std::string> read_reply();

    std::string host_;
    int port_;
    int socket_fd_ = -1;
};

}
