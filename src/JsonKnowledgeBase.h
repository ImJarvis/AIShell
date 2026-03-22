#pragma once
#include "IKnowledgeBase.h"
#include "json.hpp"

/**
 * @brief JSON File-backed implementation of IKnowledgeBase.
 * Single Responsibility Principle (SOLID): Only handles loading and querying JSON data.
 */
class JsonKnowledgeBase : public IKnowledgeBase {
public:
    JsonKnowledgeBase();
    ~JsonKnowledgeBase() override = default;

    bool Load(const std::string& filepath) override;
    std::string Query(const std::string& query) const override;

private:
    nlohmann::json m_db;
    bool m_isLoaded;
};
