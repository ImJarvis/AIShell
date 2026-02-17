//#include "LlamaManager.h"
//#include <vector>
//
//LlamaManager::LlamaManager(const std::string& modelPath) {
//    llama_backend_init();
//    auto m_params = llama_model_default_params();
//    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
//    m_ctx = nullptr;
//}
//
//LlamaManager::~LlamaManager() {
//    if (m_ctx) llama_free(m_ctx);
//    if (m_model) llama_free_model(m_model);
//    llama_backend_free();
//}
//
//std::string LlamaManager::GenerateCommand(const std::string& input) {
//    if (!m_model) return "Error: Model file not found or invalid.";
//
//    // 1. FRESH START: Recreate context to wipe memory of previous failed attempts
//    if (m_ctx) llama_free(m_ctx);
//    auto c_params = llama_context_default_params();
//    c_params.n_ctx = 2048;
//    m_ctx = llama_new_context_with_model(m_model, c_params);
//
//    if (!m_ctx) return "Error: Failed to create LLM context.";
//
//    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
//
//    // 2. UPDATED PROMPT: Using ChatML format for better reliability
//    std::string prompt = "<|system|>\nYou are a Windows Terminal. Output ONLY the CMD command. No talk.<|user|>\n" + input + "<|assistant|>\n";
//
//    // 3. TOKENIZE
//    std::vector<llama_token> tokens(prompt.length() + 32);
//    int n_tokens = llama_tokenize(vocab, prompt.c_str(), (int)prompt.length(), tokens.data(), (int)tokens.size(), true, true);
//    if (n_tokens <= 0) return "Error: Tokenization failed.";
//    tokens.resize(n_tokens);
//
//    // 4. BATCH PROCESSING
//    llama_batch batch = llama_batch_init(n_tokens + m_n_predict, 0, 1);
//    batch.n_tokens = n_tokens;
//    for (int i = 0; i < n_tokens; i++) {
//        batch.token[i] = tokens[i];
//        batch.pos[i] = i;
//        batch.n_seq_id[i] = 1;
//        batch.seq_id[i][0] = 0;
//        batch.logits[i] = (i == n_tokens - 1);
//    }
//
//    if (llama_decode(m_ctx, batch) != 0) {
//        llama_batch_free(batch);
//        return "Error: Internal decode error.";
//    }
//
//    // 5. SAMPLING LOOP
//    struct llama_sampler* sampler = llama_sampler_init_greedy();
//    std::string response = "";
//
//    for (int i = 0; i < m_n_predict; i++) {
//        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
//
//        if (llama_vocab_is_eog(vocab, id) || id == llama_vocab_nl(vocab)) break;
//
//        char buf[256];
//        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
//        if (n > 0) response.append(buf, n);
//
//        batch.n_tokens = 1;
//        batch.token[0] = id;
//        batch.pos[0] = (llama_pos)(n_tokens + i);
//        batch.n_seq_id[0] = 1;
//        batch.seq_id[0][0] = 0;
//        batch.logits[0] = true;
//
//        if (llama_decode(m_ctx, batch) != 0) break;
//    }
//
//    llama_sampler_free(sampler);
//    llama_batch_free(batch);
//
//    // Fallback if AI gets shy
//    if (response.empty()) return "Error: AI returned empty response. Try rephrasing.";
//
//    return response;
//}
//
//
////#include "LlamaManager.h"
////#include <vector>
////
////LlamaManager::LlamaManager(const std::string& modelPath) {
////    llama_backend_init();
////    auto m_params = llama_model_default_params();
////    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
////    m_ctx = nullptr;
////}
////
////LlamaManager::~LlamaManager() {
////    if (m_ctx) llama_free(m_ctx);
////    if (m_model) llama_free_model(m_model);
////    llama_backend_free();
////}
////
////std::string LlamaManager::GenerateCommand(const std::string& input) {
////    if (!m_model) return "Error: Model not loaded.";
////
////    // 1. FRESH CONTEXT: Prevents "Memory Slot" errors and cumulative context drift
////    if (m_ctx) llama_free(m_ctx);
////    auto c_params = llama_context_default_params();
////    c_params.n_ctx = 2048;
////    m_ctx = llama_new_context_with_model(m_model, c_params);
////
////    if (!m_ctx) return "Error: Context creation failed.";
////
////    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
////
////    // 2. STRICT PROMPT: Stops model from rambling
////    std::string prompt = "<|system|>\nYou are a Windows Terminal. Output ONLY the command.<|user|>\n" + input + "<|assistant|>\n";
////
////    // 3. TOKENIZE
////    std::vector<llama_token> tokens(prompt.length() + 32);
////    int n_tokens = llama_tokenize(vocab, prompt.c_str(), (int)prompt.length(), tokens.data(), (int)tokens.size(), true, true);
////    if (n_tokens <= 0) return "Error: Tokenization failed.";
////    tokens.resize(n_tokens);
////
////    // 4. BATCH SETUP: Manual init is safer for threading
////    llama_batch batch = llama_batch_init(n_tokens + m_n_predict, 0, 1);
////    batch.n_tokens = n_tokens;
////    for (int i = 0; i < n_tokens; i++) {
////        batch.token[i] = tokens[i];
////        batch.pos[i] = i;
////        batch.n_seq_id[i] = 1;
////        batch.seq_id[i][0] = 0;
////        batch.logits[i] = (i == n_tokens - 1); // Only need logits for the prediction start
////    }
////
////    if (llama_decode(m_ctx, batch) != 0) {
////        llama_batch_free(batch);
////        return "Error: Initial decode failed.";
////    }
////
////    // 5. SAMPLING
////    struct llama_sampler* sampler = llama_sampler_init_greedy();
////    std::string response = "";
////
////    for (int i = 0; i < m_n_predict; i++) {
////        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
////
////        // Stop on End-of-Generation or Newline
////        if (llama_vocab_is_eog(vocab, id) || id == llama_vocab_nl(vocab)) break;
////
////        char buf[256];
////        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
////        if (n > 0) response.append(buf, n);
////
////        // Prepare batch for next token
////        batch.n_tokens = 1;
////        batch.token[0] = id;
////        batch.pos[0] = (llama_pos)(n_tokens + i);
////        batch.n_seq_id[0] = 1;
////        batch.seq_id[0][0] = 0;
////        batch.logits[0] = true;
////
////        if (llama_decode(m_ctx, batch) != 0) break;
////    }
////
////    llama_sampler_free(sampler);
////    llama_batch_free(batch);
////
////    return response.empty() ? "Error: AI generated an empty response." : response;
////}
////####################################### GOOD WORKING ABOVE #################
//
////#include "LlamaManager.h"
////#include <vector>
////#include <iostream> // For debugging prints if needed
////
////LlamaManager::LlamaManager(const std::string& modelPath) {
////    llama_backend_init();
////
////    // 1. Load the Model ONCE (Heavy operation, keep this here)
////    auto m_params = llama_model_default_params();
////    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
////
////    // We don't initialize m_ctx here anymore, we do it per-generation
////    // to guarantee a fresh start.
////    m_ctx = nullptr;
////}
////
////LlamaManager::~LlamaManager() {
////    if (m_ctx) llama_free(m_ctx);
////    if (m_model) llama_free_model(m_model);
////    llama_backend_free();
////}
////
////std::string LlamaManager::GenerateCommand(const std::string& input) {
////    // 2. THE NUCLEAR FIX: Recreate Context
////    // Instead of fighting with llama_kv_cache_clear/seq_rm, just make a new one.
////    // This is cheap (allocates RAM only) and ensures 0% memory corruption from previous turns.
////    if (m_ctx) {
////        llama_free(m_ctx);
////        auto c_params = llama_context_default_params();
////        c_params.n_ctx = 2048;
////        m_ctx = llama_new_context_with_model(m_model, c_params);
////    }
////
////    /*auto c_params = llama_context_default_params();
////    c_params.n_ctx = 2048; // Enough memory for the prompt
////    c_params.n_batch = 2048;
////    m_ctx = llama_new_context_with_model(m_model, c_params);
////
////    if (!m_ctx) return "Error: Failed to create context";
////
////    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
////
////    // 3. Format Prompt
////    std::string formatted_prompt =
////        "<|system|>\nYou are a Windows terminal helper. Output ONLY the command.<|user|>\n"
////        + input + "<|assistant|>\n";*/
////
////    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
////
////    // 2. USE A "ZERO-SHOT" STRICT PROMPT
////    // We remove the "assistant" tag and use a direct instruction to stop it from talking.
////    std::string formatted_prompt =
////        "Instruction: Convert the following user request into a single Windows CMD command. "
////        "Do not explain. Do not say 'Thinking'. Output ONLY the command.\n\n"
////        "Request: " + input + "\n"
////        "Command: ";
////
////    // 4. Tokenization (New API Style)
////    // Calculate required size first
////    int n_tokens = llama_tokenize(vocab, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), NULL, 0, true, true);
////    if (n_tokens < 0) n_tokens = (int32_t)formatted_prompt.length() + 64; // Fallback safety
////
////    std::vector<llama_token> tokens(n_tokens);
////    int check = llama_tokenize(vocab, formatted_prompt.c_str(), (int32_t)formatted_prompt.length(), tokens.data(), (int32_t)tokens.size(), true, true);
////    if (check > 0) tokens.resize(check);
////
////    // 5. Initialize Batch (Manually to prevent Access Violation)
////    // We allocate enough space for the prompt
////    llama_batch batch = llama_batch_init((int32_t)tokens.size(), 0, 1);
////
////    // Fill batch with prompt
////    batch.n_tokens = (int32_t)tokens.size();
////    for (int i = 0; i < batch.n_tokens; i++) {
////        batch.token[i] = tokens[i];
////        batch.pos[i] = i;
////        batch.n_seq_id[i] = 1;
////        batch.seq_id[i][0] = 0;
////        batch.logits[i] = false;
////    }
////    batch.logits[batch.n_tokens - 1] = true; // We only need prediction for the last token
////
////    if (llama_decode(m_ctx, batch) != 0) {
////        llama_batch_free(batch);
////        return "Error: Decode failed";
////    }
////
////    // 6. Generation Loop
////    struct llama_sampler* sampler = llama_sampler_init_greedy();
////    std::string response = "";
////
////    for (int i = 0; i < m_n_predict; i++) {
////        // Sample next token
////        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
////
////        // Check for end of generation
////        if (llama_vocab_is_eog(vocab, id) || id == llama_vocab_nl(vocab)) break;
////
////        // Convert to string
////        char buf[128];
////        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
////        if (n > 0) response.append(buf, n);
////
////        // Update batch for next single token
////        batch.n_tokens = 1;
////        batch.token[0] = id;
////        batch.pos[0] = (int32_t)(tokens.size() + i); // Continue sequence
////        batch.n_seq_id[0] = 1;
////        batch.seq_id[0][0] = 0;
////        batch.logits[0] = true;
////
////        if (llama_decode(m_ctx, batch) != 0) break;
////    }
////
////    // 7. Cleanup
////    llama_sampler_free(sampler);
////    llama_batch_free(batch);
////
////    return response;
////}
////
////
/////*#include "LlamaManager.h"
////#include <vector>
////
////LlamaManager::LlamaManager(const std::string& modelPath) {
////    llama_backend_init();
////
////    auto m_params = llama_model_default_params();
////    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
////
////    auto c_params = llama_context_default_params();
////    c_params.n_ctx = 2048;
////    c_params.n_batch = 512;
////    m_ctx = llama_new_context_with_model(m_model, c_params);
////}
////
////LlamaManager::~LlamaManager() {
////    if (m_ctx) llama_free(m_ctx);
////    if (m_model) llama_model_free(m_model);
////    //llama_backend_free();
////}
////
////std::string LlamaManager::GenerateCommand(const std::string& input) {
////    // FIX: If llama_kv_cache_clear is not found, use this sequence:
////    // This tells the context that all sequences at all positions are now invalid/clear.
////    //llama_kv_cache_clear(m_ctx);
////
////    //llama_kv_cache_seq_rm(m_ctx, -1, -1, -1);
////
////    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
////
////    std::string formatted_prompt =
////        "<|system|>\nYou are a Windows terminal. Respond ONLY with the command.<|user|>\n"
////        + input + "<|assistant|>\n";
////
////    // Fix Tokenization for latest API
////    int n_tokens_max = (int)formatted_prompt.length() + 32;
////    std::vector<llama_token> tokens(n_tokens_max);
////    int n_tokens = llama_tokenize(vocab, formatted_prompt.c_str(), (int)formatted_prompt.length(), tokens.data(), (int)tokens.size(), true, true);
////    if (n_tokens < 0) return "Error: Tokenization failed";
////    tokens.resize(n_tokens);
////
////    // Batching
////    llama_batch batch = llama_batch_get_one(tokens.data(), (int32_t)tokens.size());
////
////    if (llama_decode(m_ctx, batch) != 0) return "Error: Decode failed";
////
////    // New Sampler API
////    struct llama_sampler* sampler = llama_sampler_init_greedy();
////
////    std::string response = "";
////    for (int i = 0; i < m_n_predict; i++) {
////        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
////
////        if (llama_vocab_is_eog(vocab, id) || id == llama_token_nl(vocab)) break;
////
////        char buf[128];
////        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
////        if (n > 0) {
////            response.append(buf, n);
////        }
////
////        batch = llama_batch_get_one(&id, 1);
////        // Position must be set for the next token in the sequence
////        batch.pos[0] = (llama_pos)(tokens.size() + i);
////
////        if (llama_decode(m_ctx, batch) != 0) break;
////    }
////
////    llama_sampler_free(sampler);
////    return response;
////}*/
//
////#############################################################################
///*
//#include "LlamaManager.h"
//#include <vector>
//
//LlamaManager::LlamaManager(const std::string& modelPath) {
//    llama_backend_init();
//    auto m_params = llama_model_default_params();
//    m_model = llama_load_model_from_file(modelPath.c_str(), m_params);
//    m_ctx = nullptr; // Start as null
//}
//
//LlamaManager::~LlamaManager() {
//    if (m_ctx) llama_free(m_ctx);
//    if (m_model) llama_free_model(m_model);
//    llama_backend_free();
//}
//
//std::string LlamaManager::GenerateCommand(const std::string& input) {
//    // 1. CRITICAL FIX: Ensure context is created if it's NULL, 
//    // or recreated if it exists to clear the "Memory Slot" error.
//    if (m_ctx) {
//        llama_free(m_ctx);
//        m_ctx = nullptr;
//    }
//
//    auto c_params = llama_context_default_params();
//    c_params.n_ctx = 2048;
//    m_ctx = llama_new_context_with_model(m_model, c_params);
//
//    // 2. SAFETY CHECK: Catch failed context creation
//    if (!m_ctx || !m_model) return "Error: Model or Context invalid.";
//
//    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
//
//    // 3. STRICT PROMPT: Force "Completion" mode to avoid conversational "Thinking..."
//    // We append "Command: " at the end to prompt the AI to start typing the command immediately.
//    std::string formatted_prompt =
//        "System: You are a Windows Terminal. Convert user requests into a single command.\n"
//        "User: " + input + "\n"
//        "Assistant: ";
//
//    // 4. Robust Tokenization
//    int n_tokens_max = (int)formatted_prompt.length() + 32;
//    std::vector<llama_token> tokens(n_tokens_max);
//    int n_tokens = llama_tokenize(vocab, formatted_prompt.c_str(), (int)formatted_prompt.length(), tokens.data(), (int)tokens.size(), true, true);
//    if (n_tokens <= 0) return "Error: Prompt too long or invalid.";
//    tokens.resize(n_tokens);
//
//    // 5. Safe Batch Initialization
//    llama_batch batch = llama_batch_init(n_tokens + m_n_predict, 0, 1);
//    batch.n_tokens = (int32_t)tokens.size();
//    for (int i = 0; i < batch.n_tokens; i++) {
//        batch.token[i] = tokens[i];
//        batch.pos[i] = i;
//        batch.n_seq_id[i] = 1;
//        batch.seq_id[i][0] = 0;
//        batch.logits[i] = false;
//    }
//    batch.logits[batch.n_tokens - 1] = true;
//
//    // Decode prompt
//    if (llama_decode(m_ctx, batch) != 0) {
//        llama_batch_free(batch);
//        return "Error: Decode failed.";
//    }
//
//    // 6. Generation with Sampler
//    struct llama_sampler* sampler = llama_sampler_init_greedy();
//    std::string response = "";
//
//    for (int i = 0; i < m_n_predict; i++) {
//        llama_token id = llama_sampler_sample(sampler, m_ctx, -1);
//
//        // Break on end of message or newline
//        if (llama_vocab_is_eog(vocab, id) || id == llama_vocab_nl(vocab)) break;
//
//        char buf[256];
//        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
//        if (n > 0) response.append(buf, n);
//
//        // Prepare for next token
//        batch.n_tokens = 1;
//        batch.token[0] = id;
//        batch.pos[0] = (llama_pos)(tokens.size() + i);
//        batch.n_seq_id[0] = 1;
//        batch.seq_id[0][0] = 0;
//        batch.logits[0] = true;
//
//        if (llama_decode(m_ctx, batch) != 0) break;
//    }
//
//    // 7. Cleanup
//    llama_sampler_free(sampler);
//    llama_batch_free(batch);
//
//    return response;
//}*/