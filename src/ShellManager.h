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

    // Checks if every command in a (potentially piped) string exists
    static bool ValidateCommand(std::string& fullCommand) {
        if (fullCommand.empty()) return false;
        
        // 0. Safety: De-wrap AI hallucinations like "[CMD] "dir" [WHY]"
        auto trimQuotes = [](std::string& s) {
            if (s.length() >= 2 && s.front() == '\"' && s.back() == '\"') {
                // Only de-wrap if there aren't OTHER quotes inside (which would mean the outer ones are likely valid)
                if (s.find('\"', 1) == s.length() - 1) {
                    s = s.substr(1, s.length() - 2);
                }
            }
        };
        trimQuotes(fullCommand);

        // 1. Split by pipes to validate the whole chain
        std::vector<std::string> segments;
        std::stringstream ss(fullCommand);
        std::string segment;
        while (std::getline(ss, segment, '|')) {
            segments.push_back(segment);
        }

        for (auto& seg : segments) {
            // Trim leading spaces for each segment
            seg.erase(0, seg.find_first_not_of(" \t"));
            if (seg.empty()) continue;

            std::stringstream segSS(seg);
            std::string baseCmd;
            segSS >> baseCmd;

            if (baseCmd.empty()) continue;

            // Handle quoted paths in segments
            if (baseCmd.front() == '\"') {
                size_t secondQuote = seg.find('\"', 1);
                if (secondQuote != std::string::npos) {
                    baseCmd = seg.substr(1, secondQuote - 1);
                }
            }

            bool found = false;

            // Whitelist of internal CMD built-ins
            static const std::vector<std::string> builtins = { 
                "dir", "copy", "del", "echo", "mkdir", "md", "rmdir", "rd", 
                "move", "ren", "type", "cls", "cd", "pushd", "popd", "ver", "vol",
                "xcopy", "robocopy", "where", "set", "path", "findstr", "find", "sort", "wmic"
            };
            
            for (const auto& b : builtins) {
                if (_stricmp(baseCmd.c_str(), b.c_str()) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Check if it's a valid executable in PATH
                std::string check = "where " + baseCmd + " >nul 2>nul";
                if (system(check.c_str()) != 0) return false; // Fail immediately if any segment is invalid
            }
        }

        return true;
    }

    // Executes a command and captures its output
    static ExecuteResult Execute(const std::string& command, std::function<void(const std::string&)> callback = nullptr) {
        ExecuteResult result;
        result.exitCode = -1;

        if (command.empty()) return result;

        // Use _popen to capture output. 
        // Heuristic: If it has common PS markers or hyphenated cmdlets, use powershell
        std::string cmd;
        bool isPowerShell = (command.find("-Item") != std::string::npos || 
                             command.find(" -") != std::string::npos || 
                             command.find("Get-") == 0 ||
                             command.find("Set-") == 0);

        if (isPowerShell) {
            // Escape double quotes for PowerShell if they exist
            std::string escaped = command;
            size_t pos = 0;
            while ((pos = escaped.find("\"", pos)) != std::string::npos) {
                escaped.insert(pos, "`");
                pos += 2;
            }
            cmd = "powershell -NoProfile -Command \"" + escaped + "\" 2>&1";
        } else {
            cmd = "(" + command + ") 2>&1"; 
        }
        
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe) {
            result.output = "Error: Failed to open pipe for execution.";
            return result;
        }

        std::array<char, 128> buffer;
        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            std::string fragment = buffer.data();
            result.output += fragment;
            if (callback) callback(fragment);
        }

        result.exitCode = _pclose(pipe);
        return result;
    }
};
