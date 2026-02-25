#pragma once
#include <string>
#include <functional>
#include <vector>
#include "llama.h"
#include "common.h"

class LlamaManager {
public:
    LlamaManager(const std::string& modelPath);
    ~LlamaManager();
    std::string GenerateCommand(const std::string& input, std::function<void(const std::string&)> callback = nullptr);
    void ResetContext();

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    std::vector<llama_token> m_historyTokens;

    // Parameters for generation
    int32_t m_n_predict = 128; 
};