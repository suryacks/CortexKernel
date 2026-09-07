#include "../include/redis_client.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>

namespace kg {

namespace {

std::optional<std::string> read_line(int fd) {
    std::string line;
    char ch = 0;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) {
            return std::nullopt;
        }
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        line.push_back(ch);
    }
}

std::optional<std::string> read_exact(int fd, size_t n) {
    std::string data;
    data.resize(n);
    size_t received = 0;
    while (received < n) {
        ssize_t r = recv(fd, data.data() + received, n - received, 0);
        if (r <= 0) {
            return std::nullopt;
        }
        received += static_cast<size_t>(r);
    }
    return data;
}

}

RedisClient::RedisClient(std::string host, int port) : host_(std::move(host)), port_(port) {}

RedisClient::~RedisClient() {
    disconnect();
}

void RedisClient::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool RedisClient::ensure_connected() {
    if (socket_fd_ >= 0) {
        return true;
    }

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    if (getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &result) != 0) {
        return false;
    }

    int fd = -1;
    for (struct addrinfo* p = result; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if (fd < 0) {
        return false;
    }

    socket_fd_ = fd;
    return true;
}

bool RedisClient::send_command(const std::vector<std::string>& args) {
    if (!ensure_connected()) {
        return false;
    }

    std::ostringstream out;
    out << "*" << args.size() << "\r\n";
    for (const auto& arg : args) {
        out << "$" << arg.size() << "\r\n" << arg << "\r\n";
    }
    std::string payload = out.str();

    size_t total_sent = 0;
    while (total_sent < payload.size()) {
        ssize_t sent = send(socket_fd_, payload.data() + total_sent, payload.size() - total_sent, 0);
        if (sent <= 0) {
            disconnect();
            return false;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return true;
}

std::optional<std::string> RedisClient::read_reply() {
    auto header = read_line(socket_fd_);
    if (!header || header->empty()) {
        disconnect();
        return std::nullopt;
    }

    char type = (*header)[0];
    std::string rest = header->substr(1);

    switch (type) {
        case '+':
        case ':':
            return rest;
        case '-':
            return std::nullopt;
        case '$': {
            int len = std::stoi(rest);
            if (len < 0) {
                return std::nullopt;
            }
            auto data = read_exact(socket_fd_, static_cast<size_t>(len));
            read_exact(socket_fd_, 2);
            return data;
        }
        default:
            return std::nullopt;
    }
}

bool RedisClient::ping() {
    if (!send_command({"PING"})) {
        return false;
    }
    auto reply = read_reply();
    return reply.has_value() && *reply == "PONG";
}

bool RedisClient::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::vector<std::string> args = {"SET", key, value};
    if (ttl_seconds > 0) {
        args.push_back("EX");
        args.push_back(std::to_string(ttl_seconds));
    }
    if (!send_command(args)) {
        return false;
    }
    auto reply = read_reply();
    return reply.has_value() && *reply == "OK";
}

std::optional<std::string> RedisClient::get(const std::string& key) {
    if (!send_command({"GET", key})) {
        return std::nullopt;
    }
    return read_reply();
}

bool RedisClient::del(const std::string& key) {
    if (!send_command({"DEL", key})) {
        return false;
    }
    return read_reply().has_value();
}

}
