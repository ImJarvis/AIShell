#include "LlamaManager.h"
#include "Logger.h"
#include <iostream>
#include <vector>

LlamaManager::LlamaManager(const std::string &modelPath,
                           const std::string &modelName)
    : m_modelName(modelName) {
  llama_backend_init();
  auto m_params = llama_model_default_params();
  m_model = llama_model_load_from_file(modelPath.c_str(), m_params);

  if (m_model) {
    auto c_params = llama_context_default_params();
    c_params.n_ctx = 2048;
    m_ctx = llama_init_from_model(m_model, c_params);
  }
  m_n_predict = 256;

  // Default to TinyLlama template as it matches current usage
  m_template = GetTinyLlamaTemplate();
}

ChatTemplate LlamaManager::GetTinyLlamaTemplate() {
  return {"<|system|>\n", "<|end|>\n",       "<|user|>\n",
          "<|end|>\n",    "<|assistant|>\n", "<|end|>\n"};
}

ChatTemplate LlamaManager::GetPhi3Template() {
  return {
      "<|system|>\n",    "<|end|>\n", "<|user|>\n", "<|end|>\n",
      "<|assistant|>\n", "<|end|>\n" // Phi-3 uses similar tags to ChatML often
  };
}

ChatTemplate LlamaManager::GetQwenTemplate() {
  return {"<|im_start|>system\n",    "<|im_end|>\n",
          "<|im_start|>user\n",      "<|im_end|>\n",
          "<|im_start|>assistant\n", "<|im_end|>\n"};
}

void LlamaManager::ResetContext() {
  m_historyTokens.clear();
  if (m_ctx) {
    // Updated API for new llama.cpp version: get memory and remove tokens for
    // sequence 0
    llama_memory_t mem = llama_get_memory(m_ctx);
    llama_memory_seq_rm(mem, 0, -1, -1);
  }
}

LlamaManager::~LlamaManager() {
  if (m_ctx)
    llama_free(m_ctx);
  if (m_model)
    llama_model_free(m_model);
  llama_backend_free();
}

std::string LlamaManager::GenerateCommand(
    const std::string &input,
    std::function<void(const std::string &)> callback) {
  if (!m_model || !m_ctx)
    return "Error: Model not loaded.";

  const struct llama_vocab *vocab = llama_model_get_vocab(m_model);

  // 1. Format the new turn using the assigned template
  std::string turnMessage = "";

  if (m_historyTokens.empty()) {
    // --- MASTER SYSTEM PROMPT (Applied to ALL models) ---
    // This ensures consistent behavior and flat JSON schema across the app.
    std::string masterPrompt =
        "You are a specialized Windows CLI AI Assistant. Your ONLY purpose is "
        "to assist with Windows command-line operations, system "
        "administration, and automation.\n"
        "CRITICAL RULES:\n"
        "1. Output exactly one FLAT JSON object: {\"cmd\": \"...\", \"why\": "
        "\"...\"}\n"
        "2. Ensure parameters are accurate for Windows. Example: use 'powercfg "
        "/batteryreport' NOT '-batterystats'.\n"
        "3. If the user says a command was wrong, listen and fix it in the NEW "
        "response.\n"
        "4. RAW commands only (no wrapping). Use '&&' or ';' for multi-step.\n"
        "5. NO conversational filler.\n"
        "6. IF THE USER ASKS ANYTHING UNRELATED to Windows CLI, REJECT it "
        "immediately with {\"cmd\": \"DENIED\", \"why\": \"...\"}.\n"
        "EXAMPLES:\n"
        "User: show my ip and active ports\n"
        "Assistant: {\"cmd\": \"ipconfig && netstat -an\", \"why\": \"Lists IP "
        "configuration and active network connections.\"}\n";

    // Model-specific logic tweaks (if any) can be appended if necessary,
    // but the Master rules above overwrite them.
    if (m_modelName.find("Phi") != std::string::npos) {
      masterPrompt += "\nNote: As a Phi model, prioritize conciseness and "
                      "avoid any preamble.";
    }

    turnMessage += m_template.systemStart + masterPrompt + m_template.systemEnd;
  }

  turnMessage += m_template.userStart + input + m_template.userEnd +
                 m_template.assistantStart;

  // Tokenize
  std::vector<llama_token> newTokens(turnMessage.length() + 8);
  int n_new = llama_tokenize(
      vocab, turnMessage.c_str(), (int)turnMessage.length(), newTokens.data(),
      (int)newTokens.size(), m_historyTokens.empty(), true);
  newTokens.resize(n_new);

  // Batch process new tokens
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

  m_historyTokens.insert(m_historyTokens.end(), newTokens.begin(),
                         newTokens.end());
  LOG_INFO("Generating response for model: " + m_modelName);

  // Sampling
  struct llama_sampler *sampler =
      llama_sampler_chain_init(llama_sampler_chain_default_params());
  llama_sampler_chain_add(
      sampler, llama_sampler_init_penalties(2048, 1.15f, 0.10f, 0.10f));
  llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
  llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
  llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.2f));
  llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

  std::string response = "";
  llama_token lastToken = -1;
  int repeatCount = 0;

  for (int i = 0; i < m_n_predict; i++) {
    llama_token id = llama_sampler_sample(sampler, m_ctx, -1);

    if (id == lastToken) {
      repeatCount++;
      if (repeatCount >= 3)
        break;
    } else {
      repeatCount = 0;
      lastToken = id;
    }

    if (llama_vocab_is_eog(vocab, id))
      break;

    char buf[256];
    int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
    if (n > 0) {
      std::string piece(buf, n);
      // Check for template end tags in response
      if (piece.find("<|") != std::string::npos)
        break;
      response += piece;
      if (callback)
        callback(piece);
    }

    m_historyTokens.push_back(id);
    batch.n_tokens = 1;
    batch.token[0] = id;
    batch.pos[0] = (llama_pos)(m_historyTokens.size() - 1);
    batch.n_seq_id[0] = 1;
    batch.seq_id[0][0] = 0;
    batch.logits[0] = true;

    if (llama_decode(m_ctx, batch) != 0)
      break;
  }

  llama_sampler_free(sampler);
  llama_batch_free(batch);
  LOG_DEBUG("Model response complete: " + response);
  return response;
}
