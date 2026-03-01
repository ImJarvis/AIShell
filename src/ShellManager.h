#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include <array>
#include <memory>
#include <functional>
#include <vector>
#include <sstream>

class ShellManager {
public:
    struct ExecuteResult {
        int exitCode;
        std::string output;
    };

    struct RiskAssessment {
        bool isValid = false;
        int riskScore = 0; // 0-10
        std::string riskReason = "";
    };

    // Scans command for high-risk patterns
    static RiskAssessment AssessCommand(std::string& fullCommand) {
        RiskAssessment assessment;
        if (fullCommand.empty()) return assessment;

        // 0. De-wrap AI hallucinations
        auto trimQuotes = [](std::string& s) {
            if (s.length() >= 2 && s.front() == '\"' && s.back() == '\"') {
                if (s.find('\"', 1) == s.length() - 1) {
                    s = s.substr(1, s.length() - 2);
                }
            }
        };
        trimQuotes(fullCommand);

        // 1. Split by ALL separators: |, &, &&, ||, ;
        std::vector<std::string> segments;
        std::string currentSegment;
        
        for (size_t i = 0; i < fullCommand.length(); ++i) {
            char c = fullCommand[i];
            bool isSeparator = false;
            
            if (c == '|' || c == ';') isSeparator = true;
            else if (c == '&' && i + 1 < fullCommand.length() && fullCommand[i+1] == '&') { isSeparator = true; i++; }
            else if (c == '&') isSeparator = true;
            else if (c == '|' && i + 1 < fullCommand.length() && fullCommand[i+1] == '|') { isSeparator = true; i++; }

            if (isSeparator) {
                if (!currentSegment.empty()) segments.push_back(currentSegment);
                currentSegment.clear();
            } else {
                currentSegment += c;
            }
        }
        if (!currentSegment.empty()) segments.push_back(currentSegment);

        assessment.isValid = true; // Assume true until check fails

        for (auto& seg : segments) {
            seg.erase(0, seg.find_first_not_of(" \t"));
            if (seg.empty()) continue;

            // --- RISK SCANNING ---
            std::string lowerSeg = seg;
            for(auto &c : lowerSeg) c = (char)tolower(c);
            
            if (lowerSeg.find("/s") != std::string::npos || lowerSeg.find("-recurse") != std::string::npos) {
                assessment.riskScore += 3;
                assessment.riskReason += "[RECURSIVE OPERATION] ";
            }
            if (lowerSeg.find("/f") != std::string::npos || lowerSeg.find("-force") != std::string::npos || lowerSeg.find("/q") != std::string::npos) {
                assessment.riskScore += 2;
                assessment.riskReason += "[FORCED/QUIET ACTION] ";
            }
            if (lowerSeg.find("del") != std::string::npos || lowerSeg.find("rmdir") != std::string::npos || lowerSeg.find("rd") != std::string::npos || lowerSeg.find("remove-") != std::string::npos) {
                assessment.riskScore += 4;
                assessment.riskReason += "[DELETION DETECTED] ";
            }

            // --- VALIDITY CHECK ---
            std::stringstream segSS(seg);
            std::string baseCmd;
            segSS >> baseCmd;
            if (baseCmd.front() == '\"') {
                size_t secondQuote = seg.find('\"', 1);
                if (secondQuote != std::string::npos) baseCmd = seg.substr(1, secondQuote - 1);
            }

            bool found = false;
            static const std::vector<std::string> builtins = { 
                "dir", "copy", "del", "echo", "mkdir", "md", "rmdir", "rd", 
                "move", "ren", "type", "cls", "cd", "pushd", "popd", "ver", "vol",
                "xcopy", "robocopy", "where", "set", "path", "findstr", "find", "sort", "wmic", "powershell", "pwsh"
            };
            
            for (const auto& b : builtins) {
                if (_stricmp(baseCmd.c_str(), b.c_str()) == 0) { found = true; break; }
            }

            if (!found) {
                std::vector<std::string> extensions = { "", ".exe", ".com", ".bat", ".cmd" };
                bool pathFound = false;
                for (const auto& ext : extensions) {
                    char fullPath[MAX_PATH]; char* filePart;
                    std::string checkCmd = baseCmd + ext;
                    if (SearchPathA(NULL, checkCmd.c_str(), NULL, MAX_PATH, fullPath, &filePart) > 0) {
                        pathFound = true; break;
                    }
                }
                if (!pathFound) { assessment.isValid = false; break; }
            }
        }

        return assessment;
    }

    // Executes a command and captures its output
    static ExecuteResult Execute(const std::string& command, std::atomic<bool>* stopSignal = nullptr, std::function<void(const std::string&)> callback = nullptr) {
        ExecuteResult result;
        result.exitCode = -1;
        if (command.empty()) return result;

        std::string fullCmdLine;
        // Smarter shell detection
        size_t firstHyphen = command.find("-");
        size_t firstSpace = command.find(" ");
        
        bool hasCmdlet = (firstHyphen != std::string::npos && (firstSpace == std::string::npos || firstHyphen < firstSpace));
        bool hasPsVar = (command.find("$") != std::string::npos);
        bool hasCmdLogic = (command.find("&&") != std::string::npos || command.find("||") != std::string::npos);
        bool startsWithPs = (command.find("powershell") == 0 || command.find("pwsh") == 0 || 
                             command.find("Get-") == 0 || command.find("Set-") == 0 || 
                             command.find("New-") == 0);
        
        // Heuristic: If it has &&, it MUST be CMD (PS 5.1 doesn't support it).
        // Otherwise, look for Get-Item, Set-Content, powershell keyword, etc.
        bool isPowerShell = !hasCmdLogic && (hasCmdlet || hasPsVar || startsWithPs);

        if (isPowerShell) {
            std::string escaped = "";
            for(char c : command) {
                if (c == '"') escaped += "`\"";
                else escaped += c;
            }
            fullCmdLine = "powershell -NoProfile -ExecutionPolicy Bypass -Command \"" + escaped + "\"";
        } else {
            fullCmdLine = "cmd /C \"" + command + "\""; 
        }

        // Win32 pipe setup
        HANDLE hRead, hWrite;
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return result;
        SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        si.cb = sizeof(STARTUPINFOA);
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = { 0 };
        // CreateProcessA requires a non-const buffer for lpCommandLine
        std::vector<char> cmdBuffer(fullCmdLine.begin(), fullCmdLine.end());
        cmdBuffer.push_back('\0');

        if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            DWORD err = GetLastError();
            std::string errMsg = "Error: Failed to launch process (Error Code: " + std::to_string(err) + ").\n" +
                                 "Command: " + fullCmdLine + "\n" +
                                 "Check if the executable exists and is in your PATH.";
            CloseHandle(hRead);
            CloseHandle(hWrite);
            result.output = errMsg;
            if (callback) callback(errMsg); 
            return result;
        }

        CloseHandle(hWrite); // Close writer in parent else read hangs

        char buffer[1024];
        DWORD bytesRead;
        while (true) {
            // Check for stop switch
            if (stopSignal && stopSignal->load()) {
                TerminateProcess(pi.hProcess, 1);
                result.output += "\n[PROCESS TERMINATED BY USER]\n";
                if (callback) callback("\n[TERMINATED]");
                break;
            }

            // Check if there is data in pipe
            DWORD avail = 0;
            if (PeekNamedPipe(hRead, NULL, 0, NULL, &avail, NULL) && avail > 0) {
                if (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    std::string frag(buffer);
                    result.output += frag;
                    if (callback) callback(frag);
                }
            } else {
                // Check if process still running
                DWORD waitRes = WaitForSingleObject(pi.hProcess, 50);
                if (waitRes != WAIT_TIMEOUT) {
                    // One last read attempt for leftover data
                    while (PeekNamedPipe(hRead, NULL, 0, NULL, &avail, NULL) && avail > 0) {
                        if (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            std::string frag(buffer);
                            result.output += frag;
                            if (callback) callback(frag);
                        } else break;
                    }
                    GetExitCodeProcess(pi.hProcess, (LPDWORD)&result.exitCode);
                    break;
                }
            }
        }

        CloseHandle(hRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return result;
    }
};
