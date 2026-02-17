//#include "Application.h"
//#include <future>
//
//
//
//Application::Application()
//{
//    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
//    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
//    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
//    m_Llama = std::make_unique<LlamaManager>("tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf");
//}
//
//void Application::Run() {
//    static char inputBuffer[512] = "";
//    static std::string aiResponse = "Hollow Shell V1.0 initialized. Awaiting Prompt...";
//    static bool scrollToBottom = false;
//
//    while (!m_Window->ShouldClose()) {
//        // Handle Window Toggle
//        if (m_Window->ProcessOSMessages(1)) {
//            if (m_Window->IsVisible()) m_Window->Hide();
//            else {
//                m_Window->Show();
//                SetForegroundWindow(m_Window->GetWin32Handle());
//            }
//        }
//
//        if (!m_Window->IsVisible()) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(16));
//            continue;
//        }
//
//        // ASYNC CHECK: Check if the AI thread is finished
//        if (m_IsThinking && m_AiThread.valid()) {
//            if (m_AiThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
//                aiResponse = m_AiThread.get();
//                m_IsThinking = false;
//                scrollToBottom = true;
//            }
//        }
//
//        // Render Prep
//        int dw, dh;
//        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
//        glViewport(0, 0, dw, dh);
//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        m_Gui->BeginFrame();
//        //ImGui::SetNextWindowPos(ImVec2(0, 0));
//        //ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));
//        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
//
//        ImGui::Begin("Terminal", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
//        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1)); // Terminal Green
//
//        ImGui::Text(">> SYSTEM_STATUS: %s", m_IsThinking ? "ANALYZING..." : "IDLE");
//        ImGui::Separator();
//
//        // Output Area
//        ImGui::BeginChild("Log", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),false,0);
//        ImGui::TextWrapped("AI: %s", aiResponse.c_str());
//        if (scrollToBottom) { ImGui::SetScrollHereY(1.0f); scrollToBottom = false; }
//        ImGui::EndChild();
//
//        // Input Area
//        ImGui::Separator();
//        if (m_IsThinking) ImGui::BeginDisabled();
//
//        if (ImGui::InputText("##in", inputBuffer, 512, ImGuiInputTextFlags_EnterReturnsTrue)) {
//            if (strlen(inputBuffer) > 0) {
//                m_IsThinking = true;
//                aiResponse = "Processing...";
//                std::string inputStr("give the correct command to "); inputStr+=inputBuffer;
//
//                m_AiThread = std::async(std::launch::async, [this, inputStr]() {
//                    return m_Llama->GenerateCommand(inputStr);
//                    });
//
//                inputBuffer[0] = '\0';
//                scrollToBottom = true;
//            }
//            ImGui::SetKeyboardFocusHere(-1);
//        }
//
//        if (m_IsThinking) ImGui::EndDisabled();
//        ImGui::PopStyleColor();
//        ImGui::End();
//
//        m_Gui->EndFrame();
//        m_Window->SwapBuffers();
//    }
//}

//
//Application::Application()
//{
//    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
//    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
//    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
//    m_Llama = std::make_unique<LlamaManager>("tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf");
//}
//
//void Application::Run() {
//    static char inputBuffer[512] = "";
//    static std::string aiResponse = "Hollow Shell V1.0 initialized. Awaiting command...";
//    static bool scrollToBottom = false;
//
//    while (!m_Window->ShouldClose()) {
//        // Handle Visibility/Hotkeys
//        if (m_Window->ProcessOSMessages(1)) {
//            if (m_Window->IsVisible()) m_Window->Hide();
//            else {
//                m_Window->Show();
//                SetForegroundWindow(m_Window->GetWin32Handle());
//            }
//        }
//
//        if (!m_Window->IsVisible()) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(16));
//            continue;
//        }
//
//        // --- ASYNC LOGIC: Check if AI is done ---
//        if (m_IsThinking && m_AiThread.valid()) {
//            if (m_AiThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
//                aiResponse = m_AiThread.get();
//                m_IsThinking = false;
//                scrollToBottom = true;
//            }
//        }
//
//        // Render Prep
//        int dw, dh;
//        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
//        glViewport(0, 0, dw, dh);
//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        m_Gui->BeginFrame();
//
//        ImGui::SetNextWindowPos(ImVec2(0, 0));
//        ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));
//
//        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
//            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
//            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;
//
//        ImGui::Begin("HollowShellTerminal", nullptr, flags);
//        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
//
//        ImGui::Text(">> SYSTEM_READY");
//        if (m_IsThinking) {
//            ImGui::SameLine();
//            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), " [ANALYZING...]");
//        }
//        ImGui::Separator();
//
//        // Terminal Display Area
//        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
//        ImGui::TextWrapped("AI: %s", aiResponse.c_str());
//        if (scrollToBottom) {
//            ImGui::SetScrollHereY(1.0f);
//            scrollToBottom = false;
//        }
//        ImGui::EndChild();
//
//        ImGui::Separator();
//
//        // Input Logic
//        ImGui::Text("Enter Prompt: "); ImGui::SameLine();
//        if (m_IsThinking) ImGui::BeginDisabled();
//
//        if (ImGui::InputText("##cmd", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
//            std::string userInput(inputBuffer);
//            if (!userInput.empty() && m_Llama) {
//                m_IsThinking = true;
//                aiResponse = "Processing request...";
//
//                // Fire and forget thread
//                m_AiThread = std::async(std::launch::async, [this, userInput]() {
//                    return m_Llama->GenerateCommand(userInput);
//                    });
//
//                memset(inputBuffer, 0, sizeof(inputBuffer));
//                scrollToBottom = true;
//            }
//            ImGui::SetKeyboardFocusHere(-1);
//        }
//
//        if (m_IsThinking) ImGui::EndDisabled();
//
//        ImGui::PopStyleColor();
//        ImGui::End();
//
//        m_Gui->EndFrame();
//        m_Window->SwapBuffers();
//    }
//}
//
//void Application::Run() {
//    // 1. Persistent state for the UI (static so they survive the loop)
//    static char inputBuffer[256] = "";
//    static std::string aiResponse = "Hollow Shell V1.0 initialized. Awaiting command...";
//    static bool scrollToBottom = false;
//
//    while (!m_Window->ShouldClose()) {
//        // Process Alt+I Hotkey
//        if (m_Window->ProcessOSMessages(1)) {
//            if (m_Window->IsVisible()) m_Window->Hide();
//            else {
//                m_Window->Show();
//                SetForegroundWindow(m_Window->GetWin32Handle());
//            }
//        }
//
//        if (!m_Window->IsVisible()) {
//            Sleep(16);
//            continue;
//        }
//
//        // --- RENDER PREPARATION ---
//        int dw, dh;
//        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
//        glViewport(0, 0, dw, dh);
//
//        // Background: 0 alpha makes the actual GL window transparent if your OS allows it
//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        m_Gui->BeginFrame();
//
//        // --- HOLLOW SHELL TERMINAL UI ---
//        // Force the window to fill the entire application area
//        ImGui::SetNextWindowPos(ImVec2(0, 0));
//        ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));
//
//        // Flags for the "Hollow" look: No background, no title, no resize
//        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
//            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
//            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;
//
//        ImGui::Begin("HollowShellTerminal", nullptr, flags);
//
//        // Set text color to Terminal Green
//        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
//
//        ImGui::Text(">> SYSTEM_READY");
//        ImGui::Separator();
//
//        // Display the AI response area
//        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
//        ImGui::TextWrapped("AI: %s", aiResponse.c_str());
//        if (scrollToBottom) {
//            ImGui::SetScrollHereY(1.0f);
//            scrollToBottom = false;
//        }
//        ImGui::EndChild();
//
//        ImGui::Separator();
//
//        // The Input Box
//        ImGui::Text("Enter Prompt: "); ImGui::SameLine();
//
//        // Focus the input box automatically when the window appears
//        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
//
//        // EnterReturnsTrue means the 'if' triggers only when you press ENTER
//        if (ImGui::InputText("##cmd", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
//
//            // --- TEST THE PROMPT ---
//            std::string userInput(inputBuffer);
//            if (!userInput.empty()) {
//                // FOOLPROOF GUARD: Ensure the pointer isn't null before calling
//                if (m_Llama) {
//                    aiResponse = m_Llama->GenerateCommand(userInput);
//                }
//                else {
//                    aiResponse = "Error: Llama system not initialized.";
//                }
//                // This call is SYNCHRONOUS. The GUI will freeze while the AI thinks.
//                //aiResponse = m_Llama->GenerateCommand(userInput);
//
//                // Clear input buffer for next command
//                memset(inputBuffer, 0, sizeof(inputBuffer));
//                scrollToBottom = true;
//            }
//
//            // Refocus after sending
//            ImGui::SetKeyboardFocusHere(-1);
//        }
//
//        ImGui::PopStyleColor(); // Restore standard colors
//
//        ImGui::End();
//
//        m_Gui->EndFrame();
//        m_Window->SwapBuffers();
//        //llama_backend_free();
//    }
//}