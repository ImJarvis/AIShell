#pragma once
#include "Window.h"
#include "InputManager.h"
#include "GuiRenderer.h"
#include <memory>
#include "LlamaManager.h" 
#include <future>

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
    //std::unique_ptr<LlamaManager> m_Llama;

    // NEW MEMBERS FOR STABILITY
    std::future<std::string> m_AiThread; // Tracks the background AI task
    bool m_IsThinking = false;           // State flag
    std::string m_aiResponse = "Hollow Shell initialized. Ready.";


};