#include "LlamaManager.h"
#include <vector>

LlamaManager::LlamaManager(const std::string& modelPath) {
    llama_backend_init();
    auto m_params = llama_model_default_params();
    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
    m_ctx = nullptr;
    m_n_predict = 128; // Default max tokens for a command response
}

LlamaManager::~LlamaManager() {
    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_free_model(m_model);
    llama_backend_free();
}

std::string LlamaManager::GenerateCommand(const std::string& input) {
    if (!m_model) return "Error: Model file not found or invalid.";

    // 1. FRESH START: Recreate context to wipe memory
    if (m_ctx) llama_free(m_ctx);
    auto c_params = llama_context_default_params();
    c_params.n_ctx = 2048;
    m_ctx = llama_new_context_with_model(m_model, c_params);

    if (!m_ctx) return "Error: Failed to create LLM context.";

    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);

    // 2. PROMPT: ChatML format
    std::string prompt = "<|system|>\nYou are a Windows Terminal. Output ONLY the CMD command. No talk.<|user|>\n" + input + "<|assistant|>\n";

    // 3. TOKENIZE
    std::vector<llama_token> tokens(prompt.length() + 32);
    int n_tokens = llama_tokenize(vocab, prompt.c_str(), (int)prompt.length(), tokens.data(), (int)tokens.size(), true, true);
    if (n_tokens <= 0) return "Error: Tokenization failed.";
    tokens.resize(n_tokens);

    // 4. BATCH PROCESSING
    llama_batch batch = llama_batch_init(2048, 0, 1); // Allocate enough for context
    batch.n_tokens = n_tokens;
    for (int i = 0; i < n_tokens; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == n_tokens - 1);
    }

    if (llama_decode(m_ctx, batch) != 0) {
        llama_batch_free(batch);
        return "Error: Internal decode error.";
    }

    // 5. SAMPLING LOOP
    struct llama_sampler* sampler = llama_sampler_init_greedy();
    std::string response = "";

    for (int i = 0; i < m_n_predict; i++) {
        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);

        // FIX: Removed "id == llama_vocab_nl(vocab)" check.
        // We ONLY stop if the model officially ends the turn (EOG).
        if (llama_vocab_is_eog(vocab, id)) break;

        char buf[256];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        if (n > 0) response.append(buf, n);

        batch.n_tokens = 1;
        batch.token[0] = id;
        batch.pos[0] = (llama_pos)(n_tokens + i);
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0] = true;

        if (llama_decode(m_ctx, batch) != 0) break;
    }

    llama_sampler_free(sampler);
    llama_batch_free(batch);

    if (response.empty()) return "Error: AI returned empty response.";

    return response;
}