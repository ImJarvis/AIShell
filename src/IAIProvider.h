#pragma once
#include <string>
#include <functional>

/**
 * @brief Interface for AI Models (Local, Cloud, Mock)
 * This allows the application to stay clean and swap models without changing core logic.
 */
class IAIProvider {
public:
    virtual ~IAIProvider() = default;

    /**
     * @brief Generates a command based on user input.
     * @param input Raw user string
     * @param callback Optional callback for token-by-token streaming
     * @return The full generated response string
     */
    virtual std::string GenerateCommand(const std::string& input, 
                                       std::function<void(const std::string& token)> callback = nullptr) = 0;

    /**
     * @brief Resets the conversation history/context
     */
    virtual void ResetContext() = 0;

    /**
     * @brief Returns a friendly name for the model (e.g. "Phi-3.5", "Gemini Pro")
     */
    virtual std::string GetModelName() const = 0;
};
