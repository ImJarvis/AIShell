#pragma once
#include "IKnowledgeBase.h"
#include <string>
#include <memory>

/**
 * @brief RAGManager handles the Retrieval step.
 * It uses an injected IKnowledgeBase to fetch relevant context for a user prompt.
 */
class RAGManager {
public:
    // Dependency Injection
    explicit RAGManager(std::shared_ptr<IKnowledgeBase> kb) : m_kb(std::move(kb)) {}

    std::string RetrieveContext(const std::string& userPrompt) const {
        if (!m_kb) return "";
        return m_kb->Query(userPrompt);
    }

private:
    std::shared_ptr<IKnowledgeBase> m_kb;
};
