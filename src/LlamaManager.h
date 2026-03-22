#pragma once
#include "IAIProvider.h"
#include "llama.h"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

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
  LlamaManager(const std::string &modelPath,
               const std::string &modelName = "Local Model");
  ~LlamaManager();

  // IAIProvider Implementation
  std::string
  GenerateCommand(const std::string &input,
                  std::function<void(const std::string &)> callback = nullptr,
                  const std::string &hints = "",
                  bool rawOnly = true) override;
  void ResetContext() override;
  std::string GetModelName() const override { return m_modelName; }

  // Template selection
  void SetTemplate(const ChatTemplate &tmpl) { m_template = tmpl; }

  // Presets
  static ChatTemplate GetGemmaTemplate();
  static ChatTemplate GetLlama3Template();
  static ChatTemplate GetQwenTemplate();

private:
  llama_model *m_model = nullptr;
  llama_context *m_ctx = nullptr;
  std::vector<llama_token> m_historyTokens;
  std::string m_modelName;
  ChatTemplate m_template;
  mutable std::recursive_mutex m_ctxMutex;

  // Parameters for generation
  int32_t m_n_predict = 256;
};