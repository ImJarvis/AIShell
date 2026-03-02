#pragma once
#include "GuiRenderer.h"
#include "IAIProvider.h"
#include "InputManager.h"
#include "ShellManager.h"
#include "Window.h"
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Application {
public:
  Application();
  virtual ~Application();
  void Run();

private:
  std::unique_ptr<Window> m_Window;
  std::unique_ptr<InputManager> m_Input;
  std::unique_ptr<GuiRenderer> m_Gui;
  std::unique_ptr<IAIProvider> m_AI;

  // Model Selection
  int m_SelectedModelIndex = 0;
  const std::vector<std::string> m_ModelOptions = {"Qwen 2.5 Coder 1.5B",
                                                   "Phi-3.5 Mini 3.8B"};
  const std::vector<std::string> m_ModelFiles = {
      "qwen2.5-coder-1.5b-instruct-q4_k_m.gguf",
      "phi-3.5-mini-instruct-q4_k_m.gguf"};

  void SwitchToModel(int index);

  const int WIDTH = 600;
  const int HEIGHT = 400;

  // AI & Execution State
  std::future<void> m_ModelLoadThread;
  std::future<std::string> m_AiThread;
  std::future<ShellManager::ExecuteResult> m_ExecThread;
  std::atomic<bool> m_IsThinking = false;
  std::atomic<bool> m_IsExecuting = false;
  std::atomic<bool> m_IsLoadingModel = false;
  std::atomic<bool> m_StopExecution = {false};
  struct ChatMessage {
    std::string role;
    std::string content;
    std::string command;
    bool isUser;
    bool hasCommand = false;
    ShellManager::RiskAssessment safety;
  };
  std::vector<ChatMessage> m_ChatHistory;

  std::mutex m_ResponseMutex;
  std::string m_aiResponse = "cmdAI initialized. Ready.";
  std::string m_CommandExplanation = "";
  std::string m_LastGeneratedCommand = "";

  bool IsRunningAsAdmin();
  bool m_IsAdmin = false;
  std::string m_TerminalOutput = "";
  std::atomic<bool> m_ScrollToBottom = false;
  ShellManager::RiskAssessment m_CurrentSafety;
  float m_PaneSplitRatio = 0.5f;
};