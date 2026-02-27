#include "Application.h"
#include "ShellManager.h"
#include "CommandParser.h"
#include "LlamaManager.h"
#include <future>

// Custom handler to prevent Ctrl+C from crashing the app
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        return TRUE; // Swallow the event
    }
    return FALSE;
}

Application::Application()
{
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    m_Window = std::make_unique<Window>(WIDTH, HEIGHT, "Hollow Shell");
    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
    
    // Initialize with default
    SwitchToModel(0);
}

void Application::SwitchToModel(int index) {
    if (index < 0 || index >= m_ModelFiles.size()) return;
    
    m_SelectedModelIndex = index;
    auto localAI = std::make_unique<LlamaManager>(m_ModelFiles[index], m_ModelOptions[index]);
    
    // Apply appropriate template
    if (m_ModelOptions[index].find("Qwen") != std::string::npos) {
        localAI->SetTemplate(LlamaManager::GetQwenTemplate());
    } else if (m_ModelOptions[index].find("Phi") != std::string::npos) {
        localAI->SetTemplate(LlamaManager::GetPhi3Template());
    } else {
        localAI->SetTemplate(LlamaManager::GetTinyLlamaTemplate());
    }
    
    m_AI = std::move(localAI);
}

void Application::Run() {
    static char inputBuffer[512] = "";

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
                std::string fullResponse = m_AiThread.get();
                ParsedCommand pc = CommandParser::Parse(fullResponse);

                if (pc.success) {
                    m_LastGeneratedCommand = pc.command;
                    m_CommandExplanation = pc.explanation;
                    
                    // Background Validation
                    m_CommandVerified = ShellManager::ValidateCommand(m_LastGeneratedCommand);
                    if (m_CommandVerified) {
                        m_aiResponse = "Command Analysis Complete. [SYSTEM: VERIFIED]";
                    } else {
                        m_aiResponse = "Command Analysis Complete. [SYSTEM: WARNING - COMMAND NOT FOUND]";
                    }
                } else {
                    m_aiResponse = fullResponse;
                    m_LastGeneratedCommand = "";
                    m_CommandExplanation = "Failed to parse command.";
                    m_CommandVerified = false;
                }

                m_IsThinking = false;
                m_ScrollToBottom = true;
            }
        }

        if (m_IsExecuting && m_ExecThread.valid()) {
            if (m_ExecThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto res = m_ExecThread.get();
                m_TerminalOutput = res.output;
                m_IsExecuting = false;
                m_ScrollToBottom = true;
            }
        }

        // 3. Render Prep
        int dw, dh;
        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
        glViewport(0, 0, dw, dh);
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
        glClear(GL_COLOR_BUFFER_BIT);

        m_Gui->BeginFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_Window->Hide();
        }

        // UI Positioning: Using FirstUseEver so it can be moved/resized by user
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        // Premium Styling
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.04f, 0.92f)); 
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.8f, 1.0f, 0.3f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        // Flags: Allow resizing
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

        // Window dragging & Resizing logic: Sync ImGui window to the OS window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Terminal", nullptr, flags);

        ImVec2 currentPos = ImGui::GetWindowPos();
        ImVec2 currentSize = ImGui::GetWindowSize();

        // Sync Position
        if (currentPos.x != 0 || currentPos.y != 0) {
            int wx, wy;
            m_Window->GetPosition(&wx, &wy);
            m_Window->SetPosition(wx + (int)currentPos.x, wy + (int)currentPos.y);
            ImGui::SetWindowPos(ImVec2(0, 0));
        }

        // Sync Size
        int fbw, fbh;
        glfwGetWindowSize(m_Window->GetNativeHandle(), &fbw, &fbh);
        if ((int)currentSize.x != fbw || (int)currentSize.y != fbh) {
            glfwSetWindowSize(m_Window->GetNativeHandle(), (int)currentSize.x, (int)currentSize.y);
        }

        // Header with Glow
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.8f, 1.0f)); 
        ImGui::Text(">> HOLLOW_SHELL_V1.2");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 220);
        
        // Model Selector Dropdown
        ImGui::PushItemWidth(200);
        if (ImGui::BeginCombo("##ModelSelect", m_ModelOptions[m_SelectedModelIndex].c_str())) {
            for (int i = 0; i < m_ModelOptions.size(); i++) {
                bool isSelected = (m_SelectedModelIndex == i);
                if (ImGui::Selectable(m_ModelOptions[i].c_str(), isSelected)) {
                    SwitchToModel(i);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::Separator();

        // Status Bar
        ImVec4 statusColor = m_IsThinking ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.4f, 1.0f);
        ImGui::TextColored(statusColor, "STATUS: %s", m_IsThinking ? "ANALYZING..." : "READY");
        
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);
        if (ImGui::SmallButton("RESET AI")) {
            m_AI->ResetContext();
            m_aiResponse = "AI Context Reset Done.";
            m_LastGeneratedCommand = "";
            m_CommandVerified = false;
        }
        ImGui::Spacing();

        // 4. Output Log
        float reservedHeight = ImGui::GetFrameHeightWithSpacing() * 2.5f;
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -reservedHeight), false, 0);

        if (m_IsThinking) {
            std::lock_guard<std::mutex> lock(m_ResponseMutex);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.4f, 1.0f), ">> ANALYZING USER INTENT...");
            if (m_aiResponse.empty()) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Starting LLM context...]");
            } else {
                // Pulse or Progress bar could go here, but a steady message is cleaner than JSON spew
                static float lastDotTime = 0;
                static int dotCount = 0;
                if (ImGui::GetTime() - lastDotTime > 0.5f) {
                    dotCount = (dotCount + 1) % 4;
                    lastDotTime = (float)ImGui::GetTime();
                }
                std::string dots = std::string(dotCount, '.');
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Processing tokens%s", dots.c_str());
            }
        } else if (!m_LastGeneratedCommand.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[PROPOSED_COMMAND]");
            
            ImGui::SameLine();
            if (m_CommandVerified) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.4f, 1.0f), "(v) SYSTEM VERIFIED");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "(x) COMMAND NOT RECOGNIZED");
            }

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
            // Copy Button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("TO CLIPBOARD")) {
                ImGui::SetClipboardText(m_LastGeneratedCommand.c_str());
            }
            ImGui::PopStyleColor();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::TextWrapped("%s", m_LastGeneratedCommand.c_str());
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.4f, 1.0f), "[SAFETY_ANALYSIS]");
            ImGui::TextWrapped("%s", m_CommandExplanation.c_str());
        } else {
            ImGui::TextWrapped("AI: %s", m_aiResponse.c_str());
        }

        if (!m_TerminalOutput.empty() || m_IsExecuting) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[TERMINAL_OUTPUT]");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            if (m_IsExecuting) {
                std::lock_guard<std::mutex> lock(m_ResponseMutex);
                ImGui::TextWrapped("%s", m_TerminalOutput.c_str());
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "> Running...");
            } else {
                std::lock_guard<std::mutex> lock(m_ResponseMutex);
                ImGui::TextWrapped("%s", m_TerminalOutput.c_str());
            }
            ImGui::PopStyleColor();
        }

        if (m_ScrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_ScrollToBottom = false;
        }
        ImGui::EndChild();

        // 4.5 Execution Action
        if (!m_LastGeneratedCommand.empty() && !m_IsThinking) {
            if (m_IsExecuting) {
                ImGui::BeginDisabled();
                ImGui::Button("EXECUTING...", ImVec2(ImGui::GetContentRegionAvail().x, 0));
                ImGui::EndDisabled();
            } else if (!m_TerminalOutput.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 0.8f));
                if (ImGui::Button("CLEAR & RETURN TO INPUT", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    m_TerminalOutput = "";
                    m_LastGeneratedCommand = "";
                    m_aiResponse = "Ready for next command.";
                }
                ImGui::PopStyleColor();
            } else {
                if (m_CommandVerified) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.3f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.4f, 1.0f));
                    if (ImGui::Button("EXECUTE COMMAND", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        m_IsExecuting = true;
                        m_TerminalOutput = ""; 
                        m_ExecThread = std::async(std::launch::async, [this]() {
                            std::string cmd = m_LastGeneratedCommand;
                            return ShellManager::Execute(cmd, [this](const std::string& fragment) {
                                std::lock_guard<std::mutex> lock(m_ResponseMutex);
                                m_TerminalOutput += fragment;
                                m_ScrollToBottom = true;
                            });
                        });
                        m_ScrollToBottom = true;
                    }
                    ImGui::PopStyleColor(2);
                } else {
                    // Cautious state for unverified commands
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button("PROCEED REGARDLESS (UNVERIFIED)", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        m_IsExecuting = true;
                        m_TerminalOutput = "";
                        m_ExecThread = std::async(std::launch::async, [this]() {
                            std::string cmd = m_LastGeneratedCommand;
                            return ShellManager::Execute(cmd, [this](const std::string& fragment) {
                                std::lock_guard<std::mutex> lock(m_ResponseMutex);
                                m_TerminalOutput += fragment;
                                m_ScrollToBottom = true;
                            });
                        });
                        m_ScrollToBottom = true;
                    }
                    ImGui::PopStyleColor(2);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Warning: This command was not found in your system path.");
                    }
                }
            }
        }
        else {
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeightWithSpacing()));
        }

        ImGui::Separator();

        // 5. Input Area
        ImGui::Text("Shell@AI:~$ "); ImGui::SameLine();

        bool isInputDisabled = m_IsThinking;
        if (isInputDisabled) ImGui::BeginDisabled();
        
        // Proper focus logic: only set it when needed
        if (ImGui::IsWindowAppearing() || (!m_IsThinking && ImGui::IsWindowFocused())) {
             if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
                ImGui::SetKeyboardFocusHere();
        }

        if (ImGui::InputText("##cmd", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string userInput(inputBuffer);
            if (!userInput.empty() && m_AI) {
                m_IsThinking = true;
                m_TerminalOutput = "";
                m_LastGeneratedCommand = "";
                m_aiResponse = ""; // Clear for streaming

                m_AiThread = std::async(std::launch::async, [this, userInput]() {
                    return m_AI->GenerateCommand(userInput, [this](const std::string& token) {
                        std::lock_guard<std::mutex> lock(m_ResponseMutex);
                        m_aiResponse += token;
                        m_ScrollToBottom = true;
                    });
                });

                memset(inputBuffer, 0, sizeof(inputBuffer));
                m_ScrollToBottom = true;
            }
        }

        if (isInputDisabled) ImGui::EndDisabled();

        ImGui::PopStyleColor(2); // WindowBg, Border
        ImGui::PopStyleVar(3);   // Rounding, BorderSize, FrameRounding
        ImGui::End();

        m_Gui->EndFrame();
        m_Window->SwapBuffers();
    }
}