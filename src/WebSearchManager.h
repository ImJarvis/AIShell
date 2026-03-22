#pragma once
#include <string>

class WebSearchManager {
public:
    // Performs a search on Search.Brave.com and extracts the top text snippets
    static std::string SearchBrave(const std::string& query);
};
