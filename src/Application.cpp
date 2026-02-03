#include "Application.h"

Application::Application() {
    // 1. Init Window
    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
    
    // 2. Init Input (Needs Win32 Handle from Window)
    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
    
    // 3. Init GUI (Needs GLFW Handle from Window)
    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
}
void Application::Run() {
    while (!m_Window->ShouldClose()) {

        // 1. Process ALL Windows messages and check for Alt+I at the same time
        // This replaces m_Input->CheckHotkey() and m_Window->PollEvents()
        if (m_Window->ProcessOSMessages(1)) { // 1 is our Hotkey ID
            if (m_Window->IsVisible()) {
                m_Window->Hide();
            }
            else {
                m_Window->Show();
                SetForegroundWindow(m_Window->GetWin32Handle());
            }
        }

        if (!m_Window->IsVisible()) {
            Sleep(16); // ~60fps check rate, uses 0% CPU
            continue;
        }

        // --- ACTIVE STATE ---
        // Escape key to hide
        if (glfwGetKey(m_Window->GetNativeHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            m_Window->Hide();
        }

        // Rendering logic...
        int dw, dh;
        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_Gui->BeginFrame();
        m_Gui->RenderUI(dw, dh);
        m_Gui->EndFrame();

        m_Window->SwapBuffers();
    }
}