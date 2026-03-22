#include "JsonKnowledgeBase.h"
#include "Logger.h"
#include <fstream>

JsonKnowledgeBase::JsonKnowledgeBase() : m_isLoaded(false) {}

bool JsonKnowledgeBase::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_INFO("Failed to open Knowledge Base: " + filepath);
        return false;
    }

    try {
        m_db = nlohmann::json::parse(file);
        m_isLoaded = true;
        LOG_INFO("Successfully loaded Knowledge Base: " + filepath);
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_INFO("JSON Parse Error in Knowledge Base: " + std::string(e.what()));
        return false;
    }
}

std::string JsonKnowledgeBase::Query(const std::string& query) const {
    if (!m_isLoaded) return "";

    std::string lowerQuery = query;
    for (auto& c : lowerQuery) c = (char)tolower(c);

    // Simple Lexical Retriever: Check if the exact key exists.
    // E.g., if query is "tar", we look for m_db["tar"]
    // To handle prompts like "How to extract tar", we check if the key is a substring of the query.
    for (auto it = m_db.begin(); it != m_db.end(); ++it) {
        std::string key = it.key();
        
        // Exact match or token match
        // A better approach is to check if the prompt contains the key surrounded by spaces or boundaries,
        // but simple substring search is fine for V1.
        std::string tokenBoundary = " " + key + " ";
        std::string tokenBoundaryEnd = " " + key;
        std::string tokenBoundaryStart = key + " ";

        if (lowerQuery == key || 
            lowerQuery.find(tokenBoundary) != std::string::npos ||
            (lowerQuery.length() >= tokenBoundaryStart.length() && lowerQuery.substr(0, tokenBoundaryStart.length()) == tokenBoundaryStart) ||
            (lowerQuery.length() >= tokenBoundaryEnd.length() && lowerQuery.substr(lowerQuery.length() - tokenBoundaryEnd.length()) == tokenBoundaryEnd)
            ) {
            
            // Limit the length to not blow out the context window (TLDR pages are usually small anyway)
            std::string result = it.value().get<std::string>();
            if (result.length() > 2000) {
                result = result.substr(0, 2000) + "...";
            }
            return "Command Documentation [" + key + "]:\n" + result;
        }
    }

    return "";
}
