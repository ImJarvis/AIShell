#pragma once
#include <string>
#include "llama.h"
#include "common.h"

class LlamaManager {
public:
    LlamaManager(const std::string& modelPath);
    ~LlamaManager();
    std::string GenerateCommand(const std::string& input);

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;

    // Parameters for generation
    int32_t m_n_predict = 32; // Shell commands are short
};