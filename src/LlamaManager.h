#pragma once
#include <string>
#include <functional>
#include <vector>
#include "IAIProvider.h"
#include "llama.h"
#include "common.h"

struct ChatTemplate {
    std::string systemStart;
    std::string systemEnd;
    std::string userStart;
    std::string userEnd;
    std::string assistantStart;
    std::string assistantEnd;
};

class LlamaManager : public IAIProvider {
public:
    LlamaManager(const std::string& modelPath, const std::string& modelName = "Local Model");
    ~LlamaManager();

    // IAIProvider Implementation
    std::string GenerateCommand(const std::string& input, std::function<void(const std::string&)> callback = nullptr) override;
    void ResetContext() override;
    std::string GetModelName() const override { return m_modelName; }

    // Template selection
    void SetTemplate(const ChatTemplate& tmpl) { m_template = tmpl; }

    // Presets
    static ChatTemplate GetTinyLlamaTemplate();
    static ChatTemplate GetPhi3Template();
    static ChatTemplate GetQwenTemplate();

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    std::vector<llama_token> m_historyTokens;
    std::string m_modelName;
    ChatTemplate m_template;

    // Parameters for generation
    int32_t m_n_predict = 256; 
};