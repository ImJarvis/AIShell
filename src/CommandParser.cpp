#include "CommandParser.h"
#include <algorithm>
#include <vector>
#include <cctype>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

ParsedCommand CommandParser::Parse(const std::string& input) 
{
    ParsedCommand result;
    if (input.empty()) return result;

    std::string fullResponse = input;

    // Console Logging
    std::cout << "\n[DEBUG] AI RAW RESPONSE:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << fullResponse << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // File Logging
    std::ofstream logFile("ai_shell_log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
        localtime_s(&tm_now, &time_t_now);
        logFile << "\n=== [" << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "] ===" << std::endl;
        logFile << fullResponse << std::endl;
        logFile.close();
    }


    auto trim = [](std::string& s) {
        if (s.empty()) return;
        
        // 1. Initial whitespace/backtick/prompt-char trim
        const std::string ignored = " \n\r\t`$>";
        size_t first = s.find_first_not_of(ignored);
        if (first == std::string::npos) { s.clear(); return; }
        size_t last = s.find_last_not_of(ignored);
        s = s.substr(first, (last - first + 1));

        // 2. Scrub Windows prompts like "D:\> dir" or "C:\some\path> dir"
        // Heuristic: If there's a '>' in the first 1/3 of the string, and it follows a colon or backslash
        size_t gt = s.find('>');
        if (gt != std::string::npos && gt < (s.length() / 2) && gt > 1) {
            std::string prefix = s.substr(0, gt);
            if (prefix.find(':') != std::string::npos || prefix.find('\\') != std::string::npos) {
                s.erase(0, gt + 1);
                // Final whitespace cleanup
                size_t nextStart = s.find_first_not_of(" \t");
                if (nextStart != std::string::npos) s.erase(0, nextStart);
                else s.clear();
            }
        }
    };

    // TIER 1: JSON Block Extraction
    size_t openB = fullResponse.find('{');
    size_t closeB = fullResponse.rfind('}');
    if (openB != std::string::npos && closeB != std::string::npos && closeB > openB) {
        std::string json = fullResponse.substr(openB, closeB - openB + 1);
        auto extract = [&](const std::string& key) {
            std::string k = "\"" + key + "\"";
            size_t kp = json.find(k);
            if (kp == std::string::npos) return std::string("");
            size_t colon = json.find(':', kp + k.length());
            if (colon == std::string::npos) return std::string("");
            size_t valStart = json.find('\"', colon);
            if (valStart == std::string::npos) return std::string("");
            std::string val; bool esc = false;
            for (size_t i = valStart + 1; i < json.length(); ++i) {
                if (esc) { val += json[i]; esc = false; }
                else if (json[i] == '\\') esc = true;
                else if (json[i] == '\"') break;
                else val += json[i];
            }
            return val;
        };
        std::string cmd = extract("cmd");
        std::string why = extract("why");
        trim(cmd); trim(why);
        if (!cmd.empty()) {
            result.command = cmd;
            result.explanation = why.empty() ? "Analyzed via structured protocol." : why;
            result.success = true;
            return result;
        }
    }

    // TIER 2: Semantic Tags [CMD] and [WHY]
    size_t cp = fullResponse.find("[CMD]");
    if (cp != std::string::npos) {
        size_t wp = fullResponse.find("[WHY]");
        size_t start = cp + 5;
        size_t end = (wp != std::string::npos && wp > cp) ? wp : fullResponse.length();
        std::string cmd = fullResponse.substr(start, end - start);
        std::string why = (wp != std::string::npos) ? fullResponse.substr(wp + 5) : "";
        trim(cmd); trim(why);
        if (!cmd.empty()) {
            result.command = cmd;
            result.explanation = why.empty() ? "Extracted from semantic tags." : why;
            result.success = true;
            return result;
        }
    }

    // TIER 3: Markdown Code Blocks
    size_t ts = fullResponse.find("```");
    if (ts != std::string::npos) {
        size_t cs = fullResponse.find_first_of("\n", ts);
        if (cs == std::string::npos) cs = ts + 3;
        else cs += 1;
        size_t te = fullResponse.find("```", cs);
        if (te != std::string::npos) {
            std::string cmd = fullResponse.substr(cs, te - cs);
            trim(cmd);
            if (!cmd.empty()) {
                result.command = cmd;
                result.explanation = "Extracted from markdown code block.";
                result.success = true;
                return result;
            }
        }
    }

    // TIER 4: Inline Code
    size_t st = fullResponse.find('`');
    if (st != std::string::npos) {
        size_t et = fullResponse.find('`', st + 1);
        if (et != std::string::npos) {
            std::string cmd = fullResponse.substr(st + 1, et - st - 1);
            trim(cmd);
            if (!cmd.empty()) {
                result.command = cmd;
                result.explanation = "Extracted from inline backticks.";
                result.success = true;
                return result;
            }
        }
    }

    // TIER 5: Fallback
    std::string firstLine;
    size_t nl = fullResponse.find_first_of("\n\r");
    if (nl != std::string::npos) firstLine = fullResponse.substr(0, nl);
    else firstLine = fullResponse;
    trim(firstLine);
    if (!firstLine.empty() && firstLine.length() > 2) {
        result.command = firstLine;
        result.explanation = "Fallback: Identified first significant line.";
        result.success = true;
    }

    return result;
}
