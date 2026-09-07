#pragma once

#include <string>
#include <utility>
#include <vector>

namespace kg::log {

enum class Level { Info, Warn, Error };

using Field = std::pair<std::string, std::string>;

void log(Level level, const std::string& message, const std::vector<Field>& fields = {});
void info(const std::string& message, const std::vector<Field>& fields = {});
void warn(const std::string& message, const std::vector<Field>& fields = {});
void error(const std::string& message, const std::vector<Field>& fields = {});

}
