#pragma once
#include "Window.h"
#include "InputManager.h"
#include "GuiRenderer.h"
#include "IAIProvider.h"
#include <memory>
#include "ShellManager.h"
#include <future>
#include <mutex>
#include <atomic>

class Application {
public:
    Application();
    void Run();

private:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<InputManager> m_Input;
    std::unique_ptr<GuiRenderer> m_Gui;
    std::unique_ptr<IAIProvider> m_AI;
    
    // Model Selection
    int m_SelectedModelIndex = 0;
    const std::vector<std::string> m_ModelOptions = { "Phi-3.5 Mini 3.8B", "Qwen 2.5 Coder 1.5B" };
    const std::vector<std::string> m_ModelFiles = { 
        "phi-3.5-mini-instruct-q4_k_m.gguf",
        "qwen2.5-coder-1.5b-instruct-q4_k_m.gguf"
    };

    void SwitchToModel(int index);

    const int WIDTH = 600;
    const int HEIGHT = 400;

    // AI & Execution State
    std::future<std::string> m_AiThread; 
    std::future<ShellManager::ExecuteResult> m_ExecThread;
    bool m_IsThinking = false;           
    bool m_IsExecuting = false;
    std::mutex m_ResponseMutex;
    
    std::string m_aiResponse = "Hollow Shell initialized. Ready.";
    std::string m_CommandExplanation = "";
    std::string m_LastGeneratedCommand = "";
    std::string m_TerminalOutput = "";
    std::atomic<bool> m_ScrollToBottom = false;
    bool m_CommandVerified = false;
};