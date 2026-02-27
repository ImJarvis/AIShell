#include "Application.h"
#include "ShellManager.h"
#include "CommandParser.h"
#include "LlamaManager.h"
#include <future>

// Custom handler to prevent Ctrl+C from crashing the app
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        return TRUE; // Swallow the event to handle shutdown ourselves via GLFW
    }
    return FALSE;
}

Application::Application()
{
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    m_Window = std::make_unique<Window>(WIDTH + 200, HEIGHT, "PowerShell on AI");
    m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
    m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());
    
    // Initialize with default (Index 0 is now Qwen)
    SwitchToModel(0);
}

Application::~Application() {
    // Graceful Termination
    m_StopExecution = true;
    m_IsThinking = false;
    
    // Wait for background tasks to complete or check termination
    if (m_ModelLoadThread.valid()) m_ModelLoadThread.wait();
    if (m_AiThread.valid()) m_AiThread.wait();
    if (m_ExecThread.valid()) m_ExecThread.wait();

    SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
}

void Application::SwitchToModel(int index) {
    if (index < 0 || index >= (int)m_ModelFiles.size()) return;
    
    m_SelectedModelIndex = index;
    m_IsLoadingModel = true;
    m_aiResponse = "Loading " + m_ModelOptions[index] + "...";

    m_ModelLoadThread = std::async(std::launch::async, [this, index]() {
        auto localAI = std::make_unique<LlamaManager>(m_ModelFiles[index], m_ModelOptions[index]);
        
        if (m_ModelOptions[index].find("Qwen") != std::string::npos) {
            localAI->SetTemplate(LlamaManager::GetQwenTemplate());
        } else if (m_ModelOptions[index].find("Phi") != std::string::npos) {
            localAI->SetTemplate(LlamaManager::GetPhi3Template());
        } else {
            localAI->SetTemplate(LlamaManager::GetTinyLlamaTemplate());
        }
        
        std::lock_guard<std::mutex> lock(m_ResponseMutex);
        m_AI = std::move(localAI);
        m_IsLoadingModel = false;
        m_aiResponse = m_ModelOptions[index] + " is ready.";
    });
}

void Application::Run() {
    static char inputBuffer[512] = "";

    while (!m_Window->ShouldClose()) {
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
                    m_CommandVerified = ShellManager::ValidateCommand(m_LastGeneratedCommand);
                    m_aiResponse = m_CommandVerified ? "Validated." : "Verification warning.";
                } else {
                    m_aiResponse = "Analysis failed."; m_LastGeneratedCommand = "";
                }
                m_IsThinking = false; m_ScrollToBottom = true;
            }
        }

        if (m_IsExecuting && m_ExecThread.valid()) {
            if (m_ExecThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto res = m_ExecThread.get();
                m_TerminalOutput = res.output; m_IsExecuting = false; m_ScrollToBottom = true;
            }
        }

        // 3. Render Prep
        int dw, dh;
        glfwGetFramebufferSize(m_Window->GetNativeHandle(), &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        m_Gui->BeginFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_Window->Hide();

        ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.01f, 0.01f, 0.02f, 0.0f)); 
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("MainPanel", nullptr, flags);

        // --- HEADER SECTION ---
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "PowerShell on AI");
        
        // CENTERED BLINKING STATUS
        {
            float time = (float)ImGui::GetTime();
            float blink = (sinf(time * 6.0f) + 1.2f) * 0.45f; // Oscillating transparency
            
            const char* statusTxt = m_IsLoadingModel ? "LOADING MODEL" : (m_IsThinking ? "THINKING" : "READY");
            ImVec4 statusColor = m_IsLoadingModel ? ImVec4(0.5f, 0.2f, 0.8f, blink) : 
                                 (m_IsThinking ? ImVec4(0.1f, 0.5f, 0.8f, blink) : ImVec4(0.0f, 0.8f, 0.3f, 1.0f));
            
            ImVec2 txtSize = ImGui::CalcTextSize(statusTxt);
            ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f - txtSize.x * 0.5f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, statusColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, statusColor);
            ImGui::Button(statusTxt, ImVec2(100, 0));
            ImGui::PopStyleColor(2);
        }

        // RIGHT GROUP: Reset AI + Dropdown
        ImGui::SameLine(ImGui::GetWindowWidth() - 320); 
        bool canOperate = !m_IsThinking && !m_IsExecuting && !m_IsLoadingModel;
        
        if (!canOperate) ImGui::BeginDisabled();
        if (ImGui::SmallButton("RESET AI")) {
            m_AI->ResetContext(); m_aiResponse = "Context Cleared.";
            m_LastGeneratedCommand = ""; m_TerminalOutput = "";
        }
        if (!canOperate) ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::PushItemWidth(200);
        if (!canOperate) ImGui::BeginDisabled();
        if (ImGui::BeginCombo("##ModelSelect", m_ModelOptions[m_SelectedModelIndex].c_str())) {
            for (int i = 0; i < (int)m_ModelOptions.size(); i++) {
                if (ImGui::Selectable(m_ModelOptions[i].c_str(), m_SelectedModelIndex == i)) SwitchToModel(i);
            }
            ImGui::EndCombo();
        }
        if (!canOperate) ImGui::EndDisabled();
        ImGui::PopItemWidth();

        ImGui::Separator();
        
        // --- MIDDLE SECTION ---
        float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.5f; 
        float availableHeight = ImGui::GetContentRegionAvail().y - footerHeight;
        float paneWidth = (ImGui::GetContentRegionAvail().x - 10) * 0.49f;

        if (m_IsLoadingModel) {
            ImGui::BeginChild("LoadingPane", ImVec2(-1, availableHeight), true);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "SYSTEM INITIALIZING...");
            ImGui::TextWrapped("Loading %s GGUF into memory. This may take a few seconds depending on your hardware...", m_ModelOptions[m_SelectedModelIndex].c_str());
            ImGui::EndChild();
        } else {
            // AI PANE
            ImGui::BeginChild("AIPane", ImVec2(paneWidth, availableHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            if (m_IsThinking) {
                float time = (float)ImGui::GetTime();
                float opacity = (sinf(time * 4.0f) + 1.0f) * 0.5f; 
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 0.5f + opacity * 0.5f), "AI is processing your intent...");
                ImGui::ProgressBar(0.9f, ImVec2(-1, 4), "");
            } else if (!m_LastGeneratedCommand.empty()) {
                bool isDenied = (m_LastGeneratedCommand == "DENIED");
                if (isDenied) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[ACCESS RESTRICTED]");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[COMMAND]");
                    ImGui::SameLine(paneWidth - 55);
                    if (ImGui::SmallButton("Copy")) ImGui::SetClipboardText(m_LastGeneratedCommand.c_str());
                    
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.12f, 1.0f));
                    ImGui::InputTextMultiline("##cmdCode", (char*)m_LastGeneratedCommand.c_str(), m_LastGeneratedCommand.length()+1, ImVec2(-1, ImGui::GetTextLineHeight()*3), ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor();
                }
                
                ImGui::TextColored(isDenied ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "[WHY]"); 
                ImGui::TextWrapped("%s", m_CommandExplanation.c_str());
            } else {
                std::lock_guard<std::mutex> lock(m_ResponseMutex);
                ImGui::TextWrapped("%s", m_aiResponse.c_str());
            }
            ImGui::EndChild();
            ImGui::SameLine();
            // TERMINAL PANE
            ImGui::BeginChild("ExecPane", ImVec2(0, availableHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[TERMINAL]");
            
            if (m_IsExecuting) {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 50);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
                if (ImGui::SmallButton("STOP")) {
                    m_StopExecution = true;
                }
                ImGui::PopStyleColor();
            }

            std::lock_guard<std::mutex> lock(m_ResponseMutex);
            ImGui::TextWrapped("%s", m_TerminalOutput.empty() ? "(No output)" : m_TerminalOutput.c_str());
            if (m_IsExecuting) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "\n> RUNNING...");
            if (m_ScrollToBottom) { ImGui::SetScrollHereY(1.0f); m_ScrollToBottom = false; }
            ImGui::EndChild();
        }

        // --- FOOTER SECTION ---
        ImGui::Spacing();
        if (!m_LastGeneratedCommand.empty() && !m_IsThinking && !m_IsLoadingModel) {
            bool isDenied = (m_LastGeneratedCommand == "DENIED");
            if (m_IsExecuting) ImGui::Button("BUSY...", ImVec2(paneWidth, 30));
            else if (isDenied) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::Button("ACTION BLOCKED", ImVec2(paneWidth, 30));
                ImGui::PopStyleColor();
            } else if (!m_TerminalOutput.empty()) {
                if (ImGui::Button("CLEAR OUTPUT", ImVec2(paneWidth, 30))) m_TerminalOutput = "";
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, m_CommandVerified ? ImVec4(0.0f, 0.5f, 0.2f, 1.0f) : ImVec4(0.6f, 0.2f, 0.1f, 1.0f));
                if (ImGui::Button("RUN COMMAND", ImVec2(paneWidth, 30))) {
                    m_IsExecuting = true; m_TerminalOutput = ""; 
                    m_StopExecution = false; // Reset kill switch
                    m_ExecThread = std::async(std::launch::async, [this]() {
                        return ShellManager::Execute(m_LastGeneratedCommand, &m_StopExecution, [this](const std::string& f) {
                            std::lock_guard<std::mutex> lock(m_ResponseMutex); m_TerminalOutput += f; m_ScrollToBottom = true;
                        });
                    });
                }
                ImGui::PopStyleColor();
            }
        } else ImGui::Dummy(ImVec2(paneWidth, 30));

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "PS_AI>"); ImGui::SameLine();
        if (!canOperate) ImGui::BeginDisabled();
        if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputTextWithHint("##cmd", "Ask intent (e.g. check system health, list my ip)...", inputBuffer, 512, ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_IsThinking = true; m_TerminalOutput = ""; m_LastGeneratedCommand = ""; m_aiResponse = "";
            std::string userIn(inputBuffer);
            m_AiThread = std::async(std::launch::async, [this, userIn]() {
                return m_AI->GenerateCommand(userIn, [this](const std::string& t) {
                    std::lock_guard<std::mutex> lock(m_ResponseMutex); m_aiResponse += t; m_ScrollToBottom = true;
                });
            });
            memset(inputBuffer, 0, 512);
        }
        ImGui::PopItemWidth();
        if (!canOperate) ImGui::EndDisabled();

        ImGui::PopStyleColor(2); ImGui::PopStyleVar(3);
        ImGui::End(); m_Gui->EndFrame(); m_Window->SwapBuffers();
    }
}
