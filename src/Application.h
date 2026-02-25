#pragma once
#include "Window.h"
#include "InputManager.h"
#include "GuiRenderer.h"
#include <memory>
#include "LlamaManager.h" 
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
    std::unique_ptr<LlamaManager> m_Llama;
    
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