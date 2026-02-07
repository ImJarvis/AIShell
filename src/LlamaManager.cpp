#include "LlamaManager.h"
#include "common.h" 
#include <stdexcept>
#include <vector>

LlamaManager::LlamaManager(const std::string& modelPath) {
    llama_backend_init();

    auto mparams = llama_model_default_params();
    // Error Fix: using llama_model_load_from_file instead of llama_load_model_from_file
    m_model = llama_model_load_from_file(modelPath.c_str(), mparams);

    if (!m_model) {
        throw std::runtime_error("Failed to load model: " + modelPath);
    }

    auto cparams = llama_context_default_params();
    cparams.n_ctx = 512;
    cparams.n_batch = 512; // Ensure batch size matches n_ctx for simplicity
    //m_ctx = llama_new_context_with_model(m_model, cparams);
    // Error Fix: using llama_init_from_model instead of llama_new_context_with_model
    m_ctx = llama_init_from_model(m_model, cparams);
}

std::string LlamaManager::GenerateCommand(const std::string& input) {
    if (!m_ctx) return "Error: Context not initialized";

    //std::string prompt = "You are a specialized Linux terminal assistant. Your ONLY job is to convert natural language into a valid shell command. Do not explain anything. Do not include markdown. If the user asks 'list files', respond with 'ls'." + input + "\nCommand:";
    std::string prompt =
        "System: You are a Linux Terminal. Convert user requests into shell commands. "
        "Respond ONLY with the command. No explanation.\n"
        "User: " + input + "\n"
        "Assistant: ";
        //"Output only the shell command for: " + input + "\nCommand:";

    // Error Fix: Correcting llama_tokenize arguments
    // We now need the vocab specifically
    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);

    // llama_tokenize usually returns the count, so we pre-allocate
    std::vector<llama_token> tokens(prompt.length() + 32);
    int n_tokens = llama_tokenize(vocab, prompt.c_str(), (int)prompt.length(), tokens.data(), (int)tokens.size(), true, false);
    tokens.resize(n_tokens);

    std::string result = "";

    // Inference setup
    for (int i = 0; i < 32; ++i) {
        // Error Fix: llama_batch_get_one takes 3 arguments now: (tokens, n_tokens, pos_0, seq_id)
        llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size());

        if (llama_decode(m_ctx, batch) != 0) break;

        // Sampling logic (using a simple greedy approach that exists in current headers)
        auto logits = llama_get_logits_ith(m_ctx, batch.n_tokens - 1);

        // Error Fix: Use the vocab for token checks
        llama_token id = 0;
        float max_logit = -1e10f;
        int n_vocab = llama_vocab_n_tokens(vocab);
        for (int v = 0; v < n_vocab; ++v) {
            if (logits[v] > max_logit) {
                max_logit = logits[v];
                id = v;
            }
        }

        if (id == llama_vocab_eos(vocab)) break;

        // Error Fix: llama_token_to_piece correction
        char buf[128];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        if (n > 0) {
            result += std::string(buf, n);
        }

        // Add to history
        tokens.push_back(id);
    }

    return result;
}

LlamaManager::~LlamaManager() {
    if (m_ctx) llama_free(m_ctx);
    // Error Fix: using llama_model_free instead of llama_free_model
    if (m_model) llama_model_free(m_model);
    llama_backend_free();
}