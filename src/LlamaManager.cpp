#include "LlamaManager.h"
#include "Logger.h"
#include "ggml-backend.h"
#include <mutex>
#include <vector>

LlamaManager::LlamaManager(const std::string &modelPath,
                           const std::string &modelName)
    : m_modelName(modelName) {
  llama_backend_init();
  ggml_backend_load_all(); // Ensure all backends are visible

  auto m_params = llama_model_default_params();
  m_params.n_gpu_layers = 0; // Default: disable GPU offloading (CPU only)

  if (llama_supports_gpu_offload()) {
    size_t n_devs = ggml_backend_dev_count();
    bool has_gpu = false;

    LOG_INFO("Investigating GPU support for build...");
    for (size_t i = 0; i < n_devs; ++i) {
      ggml_backend_dev_t dev = ggml_backend_dev_get(i);
      ggml_backend_dev_props props;
      ggml_backend_dev_get_props(dev, &props);

      LOG_INFO("Device " + std::to_string(i) + ": " + props.name + " (" +
               props.description + ")");

      // Try to find a Discrete or Integrated GPU that actually supports compute
      if (props.type == GGML_BACKEND_DEVICE_TYPE_GPU ||
          props.type == GGML_BACKEND_DEVICE_TYPE_IGPU) {
        has_gpu = true;
        LOG_INFO("Selected GPU for acceleration: " + std::string(props.name));
        break;
      }
    }

    if (has_gpu) {
      m_params.n_gpu_layers = 99; // Safe to offload
      LOG_INFO("GPU acceleration enabled (99 layers).");
    } else {
      LOG_INFO("No dedicated or integrated GPU detected. Falling back to CPU.");
    }
  } else {
    LOG_INFO("Llama.cpp build does not support GPU offloading.");
  }

  m_model = llama_model_load_from_file(modelPath.c_str(), m_params);

  if (m_model) {
    auto c_params = llama_context_default_params();
    c_params.n_ctx = 4096;
    c_params.n_batch = 2048; // Can decode up to 2048 at a time
    m_ctx = llama_init_from_model(m_model, c_params);
  }
  m_n_predict = 256;

  // Auto-detect template based on name
  std::string lowerName = m_modelName;
  for (auto &c : lowerName) c = (char)tolower(c);

  if (lowerName.find("gemma") != std::string::npos) {
    m_template = GetGemmaTemplate();
    LOG_INFO("Using Gemma prompt template.");
  } else if (lowerName.find("qwen") != std::string::npos) {
    m_template = GetQwenTemplate();
    LOG_INFO("Using Qwen (ChatML) prompt template.");
  } else if (lowerName.find("llama") != std::string::npos) {
    m_template = GetLlama3Template();
    LOG_INFO("Using Llama-3 prompt template.");
  } else {
    m_template = GetLlama3Template(); // Fallback
    LOG_INFO("Using fallback Llama-3 prompt template.");
  }
}

ChatTemplate LlamaManager::GetGemmaTemplate() {
  return {"<start_of_turn>system\n", "<end_of_turn>\n", "<start_of_turn>user\n",
          "<end_of_turn>\n", "<start_of_turn>assistant\n", "<end_of_turn>\n"};
}

ChatTemplate LlamaManager::GetLlama3Template() {
  return {"<|start_header_id|>system<|end_header_id|>\n\n",
          "<|eot_id|>",
          "<|start_header_id|>user<|end_header_id|>\n\n",
          "<|eot_id|>",
          "<|start_header_id|>assistant<|end_header_id|>\n\n",
          "<|eot_id|>"};
}

ChatTemplate LlamaManager::GetQwenTemplate() {
  return {"<|im_start|>system\n",    "<|im_end|>\n",
          "<|im_start|>user\n",      "<|im_end|>\n",
          "<|im_start|>assistant\n", "<|im_end|>\n"};
}

void LlamaManager::ResetContext() {
  std::lock_guard<std::recursive_mutex> lock(m_ctxMutex);
  m_historyTokens.clear();
  if (m_ctx) {
    // Updated API for new llama.cpp version: get memory and remove tokens for sequence 0
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

std::string LlamaManager::GenerateCommand(const std::string &input,
                              std::function<void(const std::string &)> callback,
                              const std::string &hints,
                              bool rawOnly) {
  std::lock_guard<std::recursive_mutex> lock(m_ctxMutex);
  if (!m_model || !m_ctx)
    return "Error: Model not loaded.";

  const struct llama_vocab *vocab = llama_model_get_vocab(m_model);

  // 1. Format the new turn using the assigned template
  std::string turnMessage = "";

  if (m_historyTokens.empty()) {
    // --- MASTER SYSTEM PROMPT ---
    std::string masterPrompt =
        "You are a specialized CLI Copilot. Your ONLY purpose is to translate user intent into executable Windows commands.\n\n"
        "CRITICAL RULES:\n"
        "1. RAW OUTPUT ONLY: You MUST output the pure, executable command. Absolutely NO markdown formatting, NO backticks (```), and NO explanations.\n"
        "2. SHELL PREFERENCE: Prefer modern PowerShell syntax unless CMD is explicitly requested or more appropriate for simple tasks.\n"
        "3. PIVOT RULE: If the user says 'wrong', 'error', or provides error output, DO NOT repeat your previous answer. Infer why it failed and output a different command.\n"
        "4. TRANSLATION REQUIRED: If SYSTEM REFERENCE documentation is provided, it may be based on Unix/Linux syntax. You MUST strictly translate those core concepts into valid Windows PowerShell equivalents. Never output Linux commands like 'ls', 'grep', 'tar', or 'ps'.\n\n"
        "EXAMPLES:\n"
        "User: show my ip\n"
        "Assistant: ipconfig\n"
        "User: list folders in D sorted\n"
        "Assistant: Get-ChildItem -Path D:\\ -Directory | Sort-Object Name\n";

    turnMessage += m_template.systemStart + masterPrompt + m_template.systemEnd;
  }

  // Inject RAG Hints as a temporary reference Turn if present
  if (!hints.empty()) {
    turnMessage += m_template.systemStart +
                   "SYSTEM REFERENCE (Warning: May be Unix/Linux based. Translate perfectly to native Windows PowerShell !!):\n" + hints +
                   m_template.systemEnd;
  }

  // SECURITY OPTIMIZATION: Sanitize template tokens to prevent prompt injection
  std::string sanitizedInput = input;
  std::vector<std::string> templateTags = {"<|",         "im_start", "im_end",
                                           "assistant|", "user|",    "system|"};
  for (const auto &tag : templateTags) {
    size_t pos = 0;
    while ((pos = sanitizedInput.find(tag, pos)) != std::string::npos) {
      sanitizedInput.erase(pos, tag.length());
    }
  }

  turnMessage += m_template.userStart + sanitizedInput + m_template.userEnd +
                 m_template.assistantStart;

  // Tokenize
  std::vector<llama_token> newTokens(turnMessage.length() + 8);
  int n_new = llama_tokenize(
      vocab, turnMessage.c_str(), (int)turnMessage.length(), newTokens.data(),
      (int)newTokens.size(), m_historyTokens.empty(), true);
  newTokens.resize(n_new);

  // Batch process new tokens
  llama_batch batch = llama_batch_init(n_new + 16, 0, 1);
  batch.n_tokens = n_new;
  for (int i = 0; i < n_new; i++) {
    batch.token[i] = newTokens[i];
    batch.pos[i] = (llama_pos)(m_historyTokens.size() + i);
    batch.n_seq_id[i] = 1;
    batch.seq_id[i][0] = 0;
    batch.logits[i] = (i == n_new - 1);
  }

  // We must decode in chunks if n_new > n_batch
  int n_batch_size = 2048;
  for (int i = 0; i < n_new; i += n_batch_size) {
      int chunk_size = std::min(n_batch_size, n_new - i);
      llama_batch chunk = llama_batch_init(chunk_size, 0, 1);
      chunk.n_tokens = chunk_size;
      for (int j = 0; j < chunk_size; j++) {
          chunk.token[j] = batch.token[i + j];
          chunk.pos[j] = batch.pos[i + j];
          chunk.n_seq_id[j] = 1;
          chunk.seq_id[j][0] = 0;
          chunk.logits[j] = batch.logits[i + j];
      }
      if (llama_decode(m_ctx, chunk) != 0) {
          llama_batch_free(chunk);
          llama_batch_free(batch);
          return "Error: Decode failed during chunking.";
      }
      llama_batch_free(chunk);
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
  llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.0f));
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
      // Check for template end tags OR newlines (for Raw Only mode) in response
      if (piece.find("<|") != std::string::npos ||
          (rawOnly && piece.find('\n') != std::string::npos))
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
