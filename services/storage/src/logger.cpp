#include "../include/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace kg::log {

namespace {

std::string level_to_string(Level level) {
    switch (level) {
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&now_c, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

}

void log(Level level, const std::string& message, const std::vector<Field>& fields) {
    std::ostream& out = (level == Level::Error) ? std::cerr : std::cout;
    out << "time=" << current_timestamp() << " level=" << level_to_string(level) << " msg=\"" << message << "\"";
    for (const auto& field : fields) {
        out << " " << field.first << "=\"" << field.second << "\"";
    }
    out << std::endl;
}

void info(const std::string& message, const std::vector<Field>& fields) {
    log(Level::Info, message, fields);
}

void warn(const std::string& message, const std::vector<Field>& fields) {
    log(Level::Warn, message, fields);
}

void error(const std::string& message, const std::vector<Field>& fields) {
    log(Level::Error, message, fields);
}

}
