#pragma once

#include "ll/api/data/KeyValueDB.h"
#include <vector>
#include <string>
#include <memory>


namespace lazy_bedrock_breaker {

class LazyBedrockBreakerMod {
private:
    std::unique_ptr<ll::data::KeyValueDB> mPlayerDb;
    std::unique_ptr<ll::data::KeyValueDB> mBlockDb;

public:
    inline std::unique_ptr<ll::data::KeyValueDB>& getPlayerDb() { return this->mPlayerDb; }
    inline std::unique_ptr<ll::data::KeyValueDB>& getBlockDb() { return this->mBlockDb; }
};

LazyBedrockBreakerMod& mod();

class ArrayManager {
private:
    std::vector<std::string> mArray = {"minecraft:bedrock"};
    static constexpr const char* mKey = "default";

    std::string serialize(const std::vector<std::string>& arr) const;
    std::vector<std::string> deserialize(const std::string& str) const;

public:
    ArrayManager() = default;
    
    bool loadFromDB();
    bool saveToDB();

    void add(const std::string& blockType);
    void del(const std::string& blockType);
    bool has(const std::string& blockType) const;

    const std::vector<std::string>& getAll() const;
    size_t size() const { return mArray.size(); }
};

ArrayManager& arr();

}   // namespace lazy_bedrock_breaker
