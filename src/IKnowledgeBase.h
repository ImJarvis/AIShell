#pragma once
#include <string>

/**
 * @brief Interface for Knowledge Bases used in the RAG pipeline.
 * Follows the Dependency Inversion Principle (SOLID).
 */
class IKnowledgeBase {
public:
    virtual ~IKnowledgeBase() = default;

    /**
     * @brief Load the knowledge base from a data source.
     * @param source The path or URI to the data source.
     * @return True if successful, false otherwise.
     */
    virtual bool Load(const std::string& source) = 0;

    /**
     * @brief Queries the knowledge base for a specific keyword or phrase.
     * @param query The search query.
     * @return The retrieved documentation context, or an empty string if not found.
     */
    virtual std::string Query(const std::string& query) const = 0;
};
