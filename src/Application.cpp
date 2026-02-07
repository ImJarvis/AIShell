#include "Application.h"

Application::Application() {
    // 1. Init Window
    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
    
    // 2. Init Input (Needs Win32 Handle from Window)
    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
    
    // 3. Init GUI (Needs GLFW Handle from Window)
    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());

    m_Llama = std::make_unique<LlamaManager>("tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf");
    //m_Llama->LoadModel();
}
void Application::Run() {
    // 1. Persistent state for the UI (static so they survive the loop)
    static char inputBuffer[256] = "";
    static std::string aiResponse = "Hollow Shell V1.0 initialized. Awaiting command...";
    static bool scrollToBottom = false;

    while (!m_Window->ShouldClose()) {
        // Process Alt+I Hotkey
        if (m_Window->ProcessOSMessages(1)) {
            if (m_Window->IsVisible()) m_Window->Hide();
            else {
                m_Window->Show();
                SetForegroundWindow(m_Window->GetWin32Handle());
            }
        }

        if (!m_Window->IsVisible()) {
            Sleep(16);
            continue;
        }

        // --- RENDER PREPARATION ---
        int dw, dh;
        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
        glViewport(0, 0, dw, dh);

        // Background: 0 alpha makes the actual GL window transparent if your OS allows it
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_Gui->BeginFrame();

        // --- HOLLOW SHELL TERMINAL UI ---
        // Force the window to fill the entire application area
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));

        // Flags for the "Hollow" look: No background, no title, no resize
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("HollowShellTerminal", nullptr, flags);

        // Set text color to Terminal Green
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

        ImGui::Text(">> SYSTEM_READY");
        ImGui::Separator();

        // Display the AI response area
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextWrapped("AI: %s", aiResponse.c_str());
        if (scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }
        ImGui::EndChild();

        ImGui::Separator();

        // The Input Box
        ImGui::Text("Enter Prompt: "); ImGui::SameLine();

        // Focus the input box automatically when the window appears
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();

        // EnterReturnsTrue means the 'if' triggers only when you press ENTER
        if (ImGui::InputText("##cmd", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {

            // --- TEST THE PROMPT ---
            std::string userInput(inputBuffer);
            if (!userInput.empty()) {
                // This call is SYNCHRONOUS. The GUI will freeze while the AI thinks.
                aiResponse = m_Llama->GenerateCommand(userInput);

                // Clear input buffer for next command
                memset(inputBuffer, 0, sizeof(inputBuffer));
                scrollToBottom = true;
            }

            // Refocus after sending
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::PopStyleColor(); // Restore standard colors

        ImGui::End();

        m_Gui->EndFrame();
        m_Window->SwapBuffers();
    }
}