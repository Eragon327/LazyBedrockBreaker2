#pragma once

#include <string>
#include <vector>

namespace lazy_bedrock_breaker::config {
struct Config {
    int                      version   = 1;
    std::vector<std::string> whiteList = {"minecraft:bedrock"};
};
} // namespace lazy_bedrock_breaker::config