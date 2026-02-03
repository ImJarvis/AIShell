#pragma once
#include "Window.h"
#include "InputManager.h"
#include "GuiRenderer.h"
#include <memory>

class Application {
public:
    Application();
    void Run();

private:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<InputManager> m_Input;
    std::unique_ptr<GuiRenderer> m_Gui;
    
    const int WIDTH = 600;
    const int HEIGHT = 400;
};