#include "LlamaManager.h"
#include <vector>

LlamaManager::LlamaManager(const std::string& modelPath) {
    llama_backend_init();
    auto m_params = llama_model_default_params();
    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
    
    if (m_model) {
        auto c_params = llama_context_default_params();
        c_params.n_ctx = 2048;
        m_ctx = llama_new_context_with_model(m_model, c_params);
    }
    m_n_predict = 256; 
}

void LlamaManager::ResetContext() {
    m_historyTokens.clear();
}

LlamaManager::~LlamaManager() {
    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_free_model(m_model);
    llama_backend_free();
}

std::string LlamaManager::GenerateCommand(const std::string& input, std::function<void(const std::string&)> callback) {
    if (!m_model || !m_ctx) return "Error: Model not loaded.";

    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);

    // 1. Format the new turn using ChatML (compatible with TinyLlama)
    std::string turnMessage = "";
    
    // Safety: Auto-reset if history is getting close to ctx limit (2048)
    if (m_historyTokens.size() > 1600) {
        ResetContext();
    }

    if (m_historyTokens.empty()) {
        turnMessage += "<|system|>\nYou are a Windows Systems Engineer. Convert user intent into valid structured JSON commands.\n"
                       "SCHEMA:\n"
                       "{\n  \"cmd\": \"The exact Windows command\",\n  \"why\": \"A brief safety or logic explanation\"\n}\n"
                       "RULES:\n"
                       "1. Use Windows CMD or PowerShell (e.g. Get-ChildItem). Prefer CMD for simple file tasks.\n"
                       "2. NEVER hallucinate commands. Use 'wmic' (not 'wmi'), 'findstr' (not 'grep'), etc.\n"
                       "3. Output ONLY the JSON object. No conversational filler, and NO shell prompts.\n"
                       "4. NEVER output source code (Javascript, Node.js, C++, etc.). ONLY valid JSON.\n"
                       "EXAMPLES:\n"
                       "User: list files\nAssistant: {\"cmd\": \"dir\", \"why\": \"Lists all files and folders in the current directory.\"}\n"
                       "User: check disk space\nAssistant: {\"cmd\": \"wmic logicaldisk get name\", \"why\": \"Queries Windows for drive names.\"}\n"
                       "User: find 'error' in log.txt\nAssistant: {\"cmd\": \"findstr /i \\\"error\\\" log.txt\", \"why\": \"Searches for 'error' case-insensitively in log.txt.\"}\n"
                       "<|end|>\n";
    }
    turnMessage += "<|user|>\n" + input + "<|end|>\n<|assistant|>\n";

    // 2. Tokenize with BOS if first message
    std::vector<llama_token> newTokens(turnMessage.length() + 8);
    int n_new = llama_tokenize(vocab, turnMessage.c_str(), (int)turnMessage.length(), newTokens.data(), (int)newTokens.size(), m_historyTokens.empty(), true);
    newTokens.resize(n_new);

    // 3. Batch process new tokens
    llama_batch batch = llama_batch_init(2048, 0, 1);
    batch.n_tokens = n_new;
    for (int i = 0; i < n_new; i++) {
        batch.token[i] = newTokens[i];
        batch.pos[i] = (llama_pos)(m_historyTokens.size() + i);
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == n_new - 1);
    }

    if (llama_decode(m_ctx, batch) != 0) {
        llama_batch_free(batch);
        return "Error: Decode failed.";
    }

    m_historyTokens.insert(m_historyTokens.end(), newTokens.begin(), newTokens.end());

    // 4. Advanced Sampling Chain
    struct llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_penalties(2048, 1.15f, 0.10f, 0.10f));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.2f));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    std::string response = "";
    std::vector<llama_token> generatedTokens;
    
    llama_token lastToken = -1;
    int repeatCount = 0;

    for (int i = 0; i < m_n_predict; i++) {
        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
        
        // Repetition Circuit Breaker
        if (id == lastToken) {
            repeatCount++;
            if (repeatCount >= 3) break; 
        } else {
            repeatCount = 0;
            lastToken = id;
        }

        // Check for End of Generation or ChatML end tag
        if (llama_vocab_is_eog(vocab, id)) break;

        char buf[256];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        if (n > 0) {
            std::string piece(buf, n);
            // Don't include the assistant end tag in the visible response
            if (piece.find("<|") != std::string::npos) break;
            response += piece;
            if (callback) callback(piece);
        }

        generatedTokens.push_back(id);

        batch.n_tokens = 1;
        batch.token[0] = id;
        batch.pos[0] = (llama_pos)(m_historyTokens.size());
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0] = true;

        if (llama_decode(m_ctx, batch) != 0) break;
        
        // Safety: ensure we update the history size tracking locally
        m_historyTokens.push_back(id);
    }

    llama_sampler_free(sampler);
    llama_batch_free(batch);

    if (response.empty()) return "Error: AI returned empty response. Check model prompt compatibility.";

    return response;
}