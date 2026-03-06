#include "Mod.h"
#include <algorithm>
#include <optional>
#include <string>

namespace lazy_bedrock_breaker {

LazyBedrockBreakerMod& mod() {
    static LazyBedrockBreakerMod instance;
    return instance;
}

ArrayManager& arr() {
    static ArrayManager instance;
    return instance;
}

std::string ArrayManager::serialize(const std::vector<std::string>& arr) const {
    std::string result;
    for (const std::basic_string<char>& item : arr) {
        result += item + ",";
    }
    if (!result.empty()) {
        result.pop_back(); // Remove the trailing comma
    }
    return result;
}

std::vector<std::string> ArrayManager::deserialize(const std::string& str) const {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(',');
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(',', start);
    }
    if (start < str.size()) {
        result.push_back(str.substr(start));
    }
    return result;
}

bool ArrayManager::loadFromDB() {
    std::unique_ptr<ll::data::KeyValueDB>& blockDb = mod().getBlockDb();
    if (!blockDb) return false;

    if (!blockDb->has(mKey)) return true;

    std::optional<std::string> value = blockDb->get(mKey);
    if (value) {
        mArray = deserialize(*value);
    }
    return true;
}

bool ArrayManager::saveToDB() {
    std::unique_ptr<ll::data::KeyValueDB>& blockDb = mod().getBlockDb();
    if (!blockDb) return false;

    if (mArray.empty()) {
        if (blockDb->has(mKey)) blockDb->del(mKey);
        return true;
    }

    std::string value = serialize(mArray);
    blockDb->set(mKey, value);
    return true;
}

void ArrayManager::add(const std::string& blockType) { 
    mArray.push_back(blockType); 
}

void ArrayManager::del(const std::string& blockType) {
    mArray.erase(std::remove(mArray.begin(), mArray.end(), blockType), mArray.end());
}

bool ArrayManager::has(const std::string& blockType) const {
    return std::find(mArray.begin(), mArray.end(), blockType) != mArray.end();
}

const std::vector<std::string>& ArrayManager::getAll() const {
    return mArray;
}


}   // namespace lazy_bedrock_breaker
