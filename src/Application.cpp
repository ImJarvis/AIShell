#include "Application.h"
#include "CommandFirewall.h"
#include "CommandParser.h"
#include "LlamaManager.h"
#include "ShellManager.h"
#include <future>

// Custom handler to prevent Ctrl+C from crashing the app
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
  if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT ||
      dwCtrlType == CTRL_CLOSE_EVENT) {
    return TRUE; // Swallow the event to handle shutdown ourselves via GLFW
  }
  return FALSE;
}

Application::Application() {
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

  m_Window = std::make_unique<Window>(WIDTH + 200, HEIGHT, "Terminal Co-Pilot");
  m_Input = std::make_unique<InputManager>(m_Window->GetWin32Handle());
  m_Gui = std::make_unique<GuiRenderer>(m_Window->GetNativeHandle());

  // Initialize with default (Index 0 is now Qwen)
  SwitchToModel(0);

  m_IsAdmin = IsRunningAsAdmin();
}

Application::~Application() {
  // Graceful Termination
  m_StopExecution = true;
  m_IsThinking = false;

  // Wait for background tasks to complete or check termination
  if (m_ModelLoadThread.valid())
    m_ModelLoadThread.wait();
  if (m_AiThread.valid())
    m_AiThread.wait();
  if (m_ExecThread.valid())
    m_ExecThread.wait();

  SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
}

void Application::SwitchToModel(int index) {
  if (index < 0 || index >= (int)m_ModelFiles.size())
    return;

  m_SelectedModelIndex = index;
  m_IsLoadingModel = true;
  m_aiResponse = "Loading " + m_ModelOptions[index] + "...";

  m_ModelLoadThread = std::async(std::launch::async, [this, index]() {
    auto localAI = std::make_unique<LlamaManager>(m_ModelFiles[index],
                                                  m_ModelOptions[index]);

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
    m_ChatHistory.push_back({"AI", m_aiResponse, "", false, false, {}});
  });
}

bool Application::IsRunningAsAdmin() {
  BOOL fIsRunAsAdmin = FALSE;
  HANDLE hToken = NULL;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (GetTokenInformation(hToken, TokenElevation, &elevation,
                            sizeof(elevation), &dwSize)) {
      fIsRunAsAdmin = elevation.TokenIsElevated;
    }
  }
  if (hToken)
    CloseHandle(hToken);
  return fIsRunAsAdmin;
}

void Application::Run() {
  static char inputBuffer[512] = "";

  while (!m_Window->ShouldClose()) {
    if (m_Window->ProcessOSMessages(1)) {
      if (m_Window->IsVisible())
        m_Window->Hide();
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
      if (m_AiThread.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {
        std::string fullResponse = m_AiThread.get();
        ParsedCommand pc = CommandParser::Parse(fullResponse);

        if (pc.success) {
          m_LastGeneratedCommand = pc.command;
          m_CommandExplanation = pc.explanation;
          m_CurrentSafety = ShellManager::AssessCommand(m_LastGeneratedCommand);
          m_aiResponse =
              m_CurrentSafety.isValid ? "Validated." : "Verification warning.";
        } else {
          m_aiResponse = "Analysis failed.";
          m_LastGeneratedCommand = "";
          m_CurrentSafety = {};
        }

        {
          std::lock_guard<std::mutex> lock(m_ResponseMutex);
          m_ChatHistory.push_back({"AI", pc.explanation, pc.command, false,
                                   pc.success, m_CurrentSafety});
        }

        m_IsThinking = false;
        m_ScrollToBottom = true;
      }
    }

    if (m_IsExecuting && m_ExecThread.valid()) {
      if (m_ExecThread.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {
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
    glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_Gui->BeginFrame();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
      m_Window->Hide();

    ImGui::SetNextWindowSize(ImVec2((float)dw, (float)dh));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.12f,
                                                    1.0f)); // System Dark Gray
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f); // Apple Pill Style
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("MainPanel", nullptr, flags);

    // Global Status
    bool canOperateStatus =
        !m_IsThinking && !m_IsExecuting && !m_IsLoadingModel;

    // --- TOP-LEFT: MODEL SELECTION ---
    ImGui::SetCursorPos(ImVec2(15, 10));
    ImGui::PushItemWidth(200);
    if (!canOperateStatus)
      ImGui::BeginDisabled();
    if (ImGui::BeginCombo("##ModelSelectTop",
                          m_ModelOptions[m_SelectedModelIndex].c_str())) {
      for (int i = 0; i < (int)m_ModelOptions.size(); i++) {
        if (ImGui::Selectable(m_ModelOptions[i].c_str(),
                              m_SelectedModelIndex == i))
          SwitchToModel(i);
      }
      ImGui::EndCombo();
    }
    if (!canOperateStatus)
      ImGui::EndDisabled();
    ImGui::PopItemWidth();

    if (m_IsAdmin) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 0.9f), "admin");
    }

    // MINIMALIST pulsing dot status
    {
      float time = (float)ImGui::GetTime();
      float pulse = (sinf(time * 4.0f) + 1.2f) * 0.4f;

      const char *statusLower = m_IsLoadingModel
                                    ? "loading..."
                                    : (m_IsThinking ? "thinking..." : "ready");
      ImVec4 dotColor =
          m_IsLoadingModel ? ImVec4(0.75f, 0.35f, 0.95f, pulse)
                           : (m_IsThinking ? ImVec4(0.25f, 0.6f, 1.0f, pulse)
                                           : ImVec4(0.35f, 0.85f, 0.45f, 1.0f));

      ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f - 30);
      ImVec2 dotPos = ImGui::GetCursorScreenPos();
      dotPos.y += 10;
      ImGui::GetWindowDrawList()->AddCircleFilled(dotPos, 4.0f,
                                                  ImColor(dotColor));

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 12);
      ImGui::TextColored(ImVec4(1, 1, 1, 0.5f), "%s", statusLower);
    }

    // (canOperateStatus moved to top of frame)

    // RIGHT GROUP: Reset
    {
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);

      // Reset button
      if (!canOperateStatus)
        ImGui::BeginDisabled();
      if (ImGui::Button("Reset", ImVec2(65, 26))) {
        m_AI->ResetContext();
        m_aiResponse = "Context Cleared.";
        m_LastGeneratedCommand = "";
        m_TerminalOutput = "";
        {
          std::lock_guard<std::mutex> lock(m_ResponseMutex);
          m_ChatHistory.clear();
        }
      }
      if (!canOperateStatus)
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // --- MIDDLE SECTION ---
    float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.5f;
    float availableHeight = ImGui::GetContentRegionAvail().y - footerHeight;

    // Draggable Partition Logic
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float splitHandleWidth = 10.0f;
    float minPaneWidth = 150.0f;

    float pane1Width = (totalWidth - splitHandleWidth) * m_PaneSplitRatio;
    if (pane1Width < minPaneWidth)
      pane1Width = minPaneWidth;

    float pane2Width = totalWidth - pane1Width - splitHandleWidth;
    if (pane2Width < minPaneWidth) {
      pane2Width = minPaneWidth;
      pane1Width = totalWidth - pane2Width - splitHandleWidth;
    }

    if (m_IsLoadingModel) {
      ImGui::BeginChild("LoadingPane", ImVec2(-1, availableHeight), true);
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                         "SYSTEM INITIALIZING...");
      ImGui::TextWrapped("Loading %s GGUF into memory. This may take a few "
                         "seconds depending on your hardware...",
                         m_ModelOptions[m_SelectedModelIndex].c_str());
      ImGui::EndChild();
    } else {
      // AI PANE
      float paneWidthStatus = pane1Width;
      ImGui::BeginChild("AIPane", ImVec2(paneWidthStatus, availableHeight),
                        true, 0);

      // 1. Render History
      {
        std::lock_guard<std::mutex> lock(m_ResponseMutex);
        for (const auto &msg : m_ChatHistory) {
          bool isSystemEvent =
              (msg.content.find("is ready") != std::string::npos ||
               msg.content.find("Cleared") != std::string::npos);

          if (isSystemEvent) {
            float txtWidth = ImGui::CalcTextSize(msg.content.c_str()).x;
            ImGui::SetCursorPosX((paneWidthStatus * 0.5f) - (txtWidth * 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
            ImGui::Text("%s", msg.content.c_str());
            ImGui::PopStyleColor();
          } else if (msg.isUser) {
            float width = ImGui::CalcTextSize(msg.content.c_str()).x;
            float maxWidth = paneWidthStatus * 0.75f;
            if (width > maxWidth)
              width = maxWidth;

            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - width - 15);
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "User");

            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - width - 15);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + width);
            ImGui::TextWrapped("%s", msg.content.c_str());
            ImGui::PopTextWrapPos();
          } else {
            ImGui::TextColored(ImVec4(0.7f, 0.5f, 0.95f, 1.0f), "cmdAI");
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x +
                                   (paneWidthStatus - 25));
            ImGui::TextWrapped("%s", msg.content.c_str());
            ImGui::PopTextWrapPos();

            if (msg.hasCommand) {
              bool isDenied = (msg.command == "DENIED");
              bool isFirewall = (msg.command == "FIREWALL_BLOCK");

              if (!isDenied && !isFirewall) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                      ImVec4(0.08f, 0.08f, 0.1f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                char cmdId[32];
                sprintf(cmdId, "##cmd_hist_%p", &msg);
                ImGui::InputTextMultiline(
                    cmdId, (char *)msg.command.c_str(),
                    msg.command.length() + 1,
                    ImVec2(paneWidthStatus - 85,
                           ImGui::GetTextLineHeight() * 2.5f),
                    ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();

                ImGui::Separator();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

                // --- INTEGRATED ACTION BAR ---
                if (ImGui::Button(
                        (std::string("Copy##") + std::to_string((size_t)&msg))
                            .c_str(),
                        ImVec2(60, 26))) {
                  ImGui::SetClipboardText(msg.command.c_str());
                }
                if (ImGui::IsItemHovered())
                  ImGui::SetTooltip("Copy command");

                ImGui::SameLine();
                bool execStateAtStart = m_IsExecuting;
                if (execStateAtStart)
                  ImGui::BeginDisabled();

                ImGui::PushStyleColor(ImGuiCol_Button,
                                      ImVec4(0.12f, 0.48f, 1.0f, 1.0f));
                if (ImGui::Button(
                        (std::string("Run >##") + std::to_string((size_t)&msg))
                            .c_str(),
                        ImVec2(60, 26))) {
                  m_IsExecuting = true;
                  m_TerminalOutput = "";
                  m_StopExecution = false;
                  std::string cmdToRun = msg.command;
                  m_ExecThread =
                      std::async(std::launch::async, [this, cmdToRun]() {
                        return ShellManager::Execute(
                            cmdToRun, &m_StopExecution,
                            [this](const std::string &f) {
                              std::lock_guard<std::mutex> lock(m_ResponseMutex);
                              m_TerminalOutput += f;
                              m_ScrollToBottom = true;
                            });
                      });
                }
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered())
                  ImGui::SetTooltip("Execute in terminal");
                if (execStateAtStart)
                  ImGui::EndDisabled();

                // Safety context within the block
                if (msg.safety.riskScore > 0) {
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 0.9f),
                                     "Risk: %d/10", msg.safety.riskScore);
                }
              }
            }
          }
          ImGui::Separator();
        }

        if (m_ScrollToBottom) {
          ImGui::SetScrollHereY(1.0f);
        }
      }

      if (m_IsThinking) {
        float time = (float)ImGui::GetTime();
        float opacity = (sinf(time * 4.0f) + 1.0f) * 0.5f;
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 0.5f + opacity * 0.5f),
                           "AI is processing your intent...");
        ImGui::ProgressBar(0.9f, ImVec2(-1, 4), "");
      } else if (m_ChatHistory.empty()) {
        std::lock_guard<std::mutex> lock(m_ResponseMutex);
        if (!m_aiResponse.empty()) {
          ImGui::TextWrapped("%s", m_aiResponse.c_str());
        }
      }
      ImGui::EndChild();

      // THE DRAGGABLE SPLITTER
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.1f, 0.4f, 1.0f, 1.0f));
      ImGui::Button("##Splitter", ImVec2(splitHandleWidth, availableHeight));
      if (ImGui::IsItemActive()) {
        m_PaneSplitRatio += ImGui::GetIO().MouseDelta.x / totalWidth;
        if (m_PaneSplitRatio < 0.1f)
          m_PaneSplitRatio = 0.1f;
        if (m_PaneSplitRatio > 0.9f)
          m_PaneSplitRatio = 0.9f;
      }
      ImGui::PopStyleColor(2);
      ImGui::SameLine();

      // TERMINAL PANE
      float paneWidth2Status = pane2Width;
      ImGui::BeginGroup();
      // Fixed Header Area
      ImGui::BeginChild("ExecHeader", ImVec2(paneWidth2Status, 35), true,
                        ImGuiWindowFlags_NoScrollbar);

      // Terminal Label (Static)
      ImGui::SetCursorPos(ImVec2(10, 8));
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "TERMINAL");

      // Stop Button - Centralized Pro style
      if (m_IsExecuting) {
        ImGui::SameLine((paneWidth2Status * 0.5f) - 40);
        float time = (float)ImGui::GetTime();
        float pulse = (sinf(time * 8.0f) + 1.2f) * 0.5f;
        ImVec4 stopColor = ImVec4(0.9f, 0.2f, 0.2f, 0.3f + pulse * 0.3f);
        ImGui::PushStyleColor(ImGuiCol_Button, stopColor);
        if (ImGui::Button("Stop", ImVec2(80, 24)))
          m_StopExecution = true;
        ImGui::PopStyleColor();
      }

      // Copy Button - Right Aligned
      ImGui::SameLine(paneWidth2Status - 120);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.6f));
      if (ImGui::Button("Copy Output", ImVec2(100, 26))) {
        ImGui::SetClipboardText(m_TerminalOutput.c_str());
      }
      ImGui::PopStyleColor();
      ImGui::EndChild();

      // Scrolling Output Area
      float headerHeight = 35.0f;
      float spacing = ImGui::GetStyle().ItemSpacing.y;
      ImGui::BeginChild("ExecContent",
                        ImVec2(0, availableHeight - headerHeight - spacing),
                        true, 0);
      {
        std::lock_guard<std::mutex> lock(m_ResponseMutex);
        std::string outTxt =
            m_TerminalOutput.empty() ? "(No output)" : m_TerminalOutput;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::InputTextMultiline("##term_out", (char *)outTxt.c_str(),
                                  outTxt.length() + 1, ImVec2(-1, -1),
                                  ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(2);
      }
      if (m_IsExecuting)
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "\n> RUNNING...");
      if (m_ScrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;
      }
      ImGui::EndChild();
      ImGui::EndGroup();
    }

    // --- FOOTER: UNIFIED COMMAND BAR ---
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 75);
    float barWidth = totalWidth * 0.85f;
    ImGui::SetCursorPosX((totalWidth - barWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));

    ImGui::BeginChild("CommandBar", ImVec2(barWidth, 60), true,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoBackground);

    if (!canOperateStatus)
      ImGui::BeginDisabled();

    ImGui::SetCursorPosY(10); // Center items vertically in the 60px child
    ImGui::SetNextItemWidth(barWidth - 110);
    ImGui::SetCursorPosX(15);
    bool executePressed = false;
    if (ImGui::InputTextWithHint(
            "##cmd", "Ask intent (e.g. check system health, list my ip)...",
            inputBuffer, 512, ImGuiInputTextFlags_EnterReturnsTrue)) {
      executePressed = true;
    }

    ImGui::SameLine(barWidth - 75);
    ImGui::SetCursorPosY(10); // Match input vertical alignment
    ImGui::PushStyleColor(ImGuiCol_Button,
                          m_IsThinking ? ImVec4(0.2f, 0.2f, 0.2f, 1.0f)
                                       : ImVec4(0.12f, 0.48f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 22.0f);
    if (ImGui::Button("Send", ImVec2(65, 40))) {
      executePressed = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (executePressed) {
      std::string userIn(inputBuffer);
      auto firewallRes = CommandFirewall::Assess(userIn);
      {
        std::lock_guard<std::mutex> lock(m_ResponseMutex);
        m_ChatHistory.push_back({"User", userIn, "", true, false, {}});
      }

      if (firewallRes.blocked) {
        m_aiResponse = "";
        m_LastGeneratedCommand = "FIREWALL_BLOCK";
        m_CommandExplanation = firewallRes.reason;
        {
          std::lock_guard<std::mutex> lock(m_ResponseMutex);
          m_ChatHistory.push_back(
              {"AI", firewallRes.reason, "", false, false, {}});
        }
      } else {
        m_IsThinking = true;
        m_aiResponse = "";
        m_AiThread = std::async(std::launch::async, [this, userIn]() {
          return m_AI->GenerateCommand(userIn, [this](const std::string &t) {
            std::lock_guard<std::mutex> lock(m_ResponseMutex);
            m_aiResponse += t;
            m_ScrollToBottom = true;
          });
        });
      }
      memset(inputBuffer, 0, 512);
    }
    if (!canOperateStatus)
      ImGui::EndDisabled();
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(5);
    ImGui::End();
    m_Gui->EndFrame();
    m_Window->SwapBuffers();
  }
}
