#include "Application.h"
#include <future>

Application::Application()
{
    // If you want the window to be resizeable by the OS, ensure your Window class allows it.
    // Assuming standard GLFW/Win32 window creation here.
    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
    m_Llama = std::make_unique<LlamaManager>("tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf");
}

void Application::Run() {
    static char inputBuffer[512] = "";
    static std::string aiResponse = "Hollow Shell V1.0 initialized. Awaiting Prompt...";
    static bool scrollToBottom = false;

    while (!m_Window->ShouldClose()) {
        // 1. OS Message Handling
        if (m_Window->ProcessOSMessages(1)) {
            if (m_Window->IsVisible()) m_Window->Hide();
            else {
                m_Window->Show();
                SetForegroundWindow(m_Window->GetWin32Handle());
            }
        }

        if (!m_Window->IsVisible()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // 2. Async Response Handling
        if (m_IsThinking && m_AiThread.valid()) {
            if (m_AiThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                aiResponse = m_AiThread.get();
                m_IsThinking = false;
                scrollToBottom = true;
            }
        }

        // 3. Render Prep
        int dw, dh;
        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Solid black background (change alpha to 0.0f for transparent)
        glClear(GL_COLOR_BUFFER_BIT);

        m_Gui->BeginFrame();

        // --- SENIOR FIX: DOCKING LOGIC ---
        // Instead of a floating window, we anchor the UI to the OS Window Viewport.
        // This means the "Black Window" IS the terminal.
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        // Flags: NoTitleBar (because the OS has one), NoResize (it resizes with OS window), NoMove (it stays docked)
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground;

        ImGui::Begin("Terminal", nullptr, flags);

        // Styling
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Matrix Green

        ImGui::Text(">> SYSTEM_STATUS: %s", m_IsThinking ? "ANALYZING..." : "ONLINE");
        ImGui::Separator();

        // 4. Output Log
        // We use 0 flags to allow standard scrolling behavior.
        // -GetFrameHeightWithSpacing() reserves exactly enough room for the input box at the bottom.
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, 0);

        ImGui::TextWrapped("AI: %s", aiResponse.c_str());

        if (scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom = false;
        }
        ImGui::EndChild();

        ImGui::Separator();

        // 5. Input Area
        ImGui::Text("User@Shell:~$ "); ImGui::SameLine();

        if (m_IsThinking) ImGui::BeginDisabled();

        // Auto-focus logic
        if (ImGui::IsWindowAppearing() || !m_IsThinking) ImGui::SetKeyboardFocusHere();

        if (ImGui::InputText("##cmd", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string userInput(inputBuffer);
            if (!userInput.empty() && m_Llama) {
                m_IsThinking = true;
                aiResponse = "Processing...";

                // Note: Improved prompt injection
                std::string inputStr = userInput;

                m_AiThread = std::async(std::launch::async, [this, inputStr]() {
                    return m_Llama->GenerateCommand(inputStr);
                    });

                memset(inputBuffer, 0, sizeof(inputBuffer));
                scrollToBottom = true;
            }
            ImGui::SetKeyboardFocusHere(-1);
        }

        if (m_IsThinking) ImGui::EndDisabled();

        ImGui::PopStyleColor();
        ImGui::End();

        m_Gui->EndFrame();
        m_Window->SwapBuffers();
    }
}