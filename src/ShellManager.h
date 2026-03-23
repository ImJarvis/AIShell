#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

class ShellManager {
private:
  static std::string ps_base64_encode(const std::string& command) {
      if (command.empty()) return "";
      int wlen = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
      if (wlen <= 0) return "";
      std::vector<wchar_t> wbuf(wlen);
      MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, wbuf.data(), wlen);
      
      int byteLen = (wlen - 1) * sizeof(wchar_t);
      const unsigned char* buf = (const unsigned char*)wbuf.data();
      
      static const std::string base64_chars = 
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz"
          "0123456789+/";
          
      std::string ret;
      int i = 0, j = 0;
      unsigned char char_array_3[3];
      unsigned char char_array_4[4];

      while (byteLen--) {
          char_array_3[i++] = *(buf++);
          if (i == 3) {
              char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
              char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
              char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
              char_array_4[3] = char_array_3[2] & 0x3f;
              for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
              i = 0;
          }
      }
      if (i) {
          for(j = i; j < 3; j++) char_array_3[j] = '\0';
          char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
          char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
          char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
          char_array_4[3] = char_array_3[2] & 0x3f;
          for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
          while((i++ < 3)) ret += '=';
      }
      return ret;
  }

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
  static RiskAssessment AssessCommand(std::string &fullCommand) {
    RiskAssessment assessment;
    if (fullCommand.empty())
      return assessment;

    // 0. De-wrap AI hallucinations
    auto trimQuotes = [](std::string &s) {
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

      if (c == '|' || c == ';')
        isSeparator = true;
      else if (c == '&' && i + 1 < fullCommand.length() &&
               fullCommand[i + 1] == '&') {
        isSeparator = true;
        i++;
      } else if (c == '&')
        isSeparator = true;
      else if (c == '|' && i + 1 < fullCommand.length() &&
               fullCommand[i + 1] == '|') {
        isSeparator = true;
        i++;
      }

      if (isSeparator) {
        if (!currentSegment.empty())
          segments.push_back(currentSegment);
        currentSegment.clear();
      } else {
        currentSegment += c;
      }
    }
    if (!currentSegment.empty())
      segments.push_back(currentSegment);

    assessment.isValid = true; // Assume true until check fails

    for (auto &seg : segments) {
      seg.erase(0, seg.find_first_not_of(" \t"));
      if (seg.empty())
        continue;

      // --- RISK SCANNING ---
      std::string lowerSeg = seg;
      for (auto &c : lowerSeg)
        c = (char)tolower(c);

      if (lowerSeg.find("/s") != std::string::npos ||
          lowerSeg.find("-recurse") != std::string::npos) {
        assessment.riskScore += 3;
        assessment.riskReason += "[RECURSIVE OPERATION] ";
      }
      if (lowerSeg.find("/f") != std::string::npos ||
          lowerSeg.find("-force") != std::string::npos ||
          lowerSeg.find("/q") != std::string::npos) {
        assessment.riskScore += 2;
        assessment.riskReason += "[FORCED/QUIET ACTION] ";
      }
      if (lowerSeg.find("del") != std::string::npos ||
          lowerSeg.find("rmdir") != std::string::npos ||
          lowerSeg.find("rd") != std::string::npos ||
          lowerSeg.find("remove-") != std::string::npos) {
        assessment.riskScore += 4;
        assessment.riskReason += "[DELETION DETECTED] ";
      }

      // --- VALIDITY CHECK ---
      std::stringstream segSS(seg);
      std::string baseCmd;
      segSS >> baseCmd;
      if (baseCmd.front() == '\"') {
        size_t secondQuote = seg.find('\"', 1);
        if (secondQuote != std::string::npos)
          baseCmd = seg.substr(1, secondQuote - 1);
      }

      bool found = false;
      static const std::vector<std::string> builtins = {
          "dir",  "copy",    "del",  "echo",  "mkdir",    "md",         "rmdir",
          "rd",   "move",    "ren",  "type",  "cls",      "cd",         "pushd",
          "popd", "ver",     "vol",  "xcopy", "robocopy", "where",      "set",
          "path", "findstr", "find", "sort",  "wmic",     "powershell", "pwsh"};

      for (const auto &b : builtins) {
        if (_stricmp(baseCmd.c_str(), b.c_str()) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        bool pathFound = false;
        if (baseCmd.find('-') != std::string::npos) {
          // It's likely a PowerShell Cmdlet (e.g., Get-Process), assume valid
          pathFound = true;
        } else {
          std::vector<std::string> extensions = {"", ".exe", ".com", ".bat",
                                                 ".cmd"};
          for (const auto &ext : extensions) {
            char fullPath[MAX_PATH];
            char *filePart;
            std::string checkCmd = baseCmd + ext;
            if (SearchPathA(NULL, checkCmd.c_str(), NULL, MAX_PATH, fullPath,
                            &filePart) > 0) {
              pathFound = true;
              break;
            }
          }
        }
        if (!pathFound) {
          assessment.isValid = false;
          break;
        }
      }
    }

    return assessment;
  }

  // Executes a command and captures its output
  static ExecuteResult
  Execute(const std::string &command, std::atomic<bool> *stopSignal = nullptr,
          std::function<void(const std::string &)> callback = nullptr) {
    ExecuteResult result;
    result.exitCode = -1;
    if (command.empty())
      return result;

    std::string fullCmdLine;
    // Smarter shell detection
    size_t firstHyphen = command.find("-");
    size_t firstSpace = command.find(" ");

    bool hasCmdlet =
        (firstHyphen != std::string::npos &&
         (firstSpace == std::string::npos || firstHyphen < firstSpace));
    bool hasPsVar = (command.find("$") != std::string::npos);
    bool hasCmdLogic = (command.find("&&") != std::string::npos ||
                        command.find("||") != std::string::npos);
    bool startsWithPs =
        (command.find("powershell") == 0 || command.find("pwsh") == 0 ||
         command.find("Get-") == 0 || command.find("Set-") == 0 ||
         command.find("New-") == 0);

    // Heuristic: If it has &&, it MUST be CMD (PS 5.1 doesn't support it).
    // Otherwise, look for Get-Item, Set-Content, powershell keyword, etc.
    bool isPowerShell = !hasCmdLogic && (hasCmdlet || hasPsVar || startsWithPs);

    if (isPowerShell) {
      // Encode as Base64 to entirely bypass CMD and powershell quotation issues.
      // We wrap the raw command to suppress CLIXML progress noise and force errors into plaintext streams.
      // RCA FIX: Use $global:error to catch cmdlet failures (like Get-WmiObject) that don't set $LASTEXITCODE.
      std::string safeWrapper = "$ProgressPreference='SilentlyContinue'; $global:error.Clear(); $out = & { " + command + " } 2>&1; $failed = ($global:error.Count -gt 0 -or $LASTEXITCODE -ne 0); $out | Out-String -Stream; if ($failed) { exit 1 }";
      std::string encodedCmd = ps_base64_encode(safeWrapper);
      fullCmdLine =
          "powershell -NoProfile -ExecutionPolicy Bypass -EncodedCommand " + encodedCmd;
    } else {
      // Use standard CMD /C. The Outer quotes are handled by the system
      // when passed to CreateProcess via cmdBuffer.
      fullCmdLine = "cmd /C \"" + command + "\"";
    }

    // Win32 pipe setup
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
      return result;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};
    // CreateProcessA requires a non-const buffer for lpCommandLine
    std::vector<char> cmdBuffer(fullCmdLine.begin(), fullCmdLine.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
      DWORD err = GetLastError();
      std::string errMsg =
          "Error: Failed to launch process (Error Code: " +
          std::to_string(err) + ").\n" + "Command: " + fullCmdLine + "\n" +
          "Check if the executable exists and is in your PATH.";
      CloseHandle(hRead);
      CloseHandle(hWrite);
      result.output = errMsg;
      if (callback)
        callback(errMsg);
      return result;
    }

    CloseHandle(hWrite); // Close writer in parent else read hangs

    std::ostringstream ssOutput;
    std::mutex outputMutex;
    std::atomic<bool> threadFinished{false};

    // Thread to read from pipe
    std::thread reader([hRead, &ssOutput, &outputMutex, &threadFinished,
                        callback]() {
      char buffer[4096];
      DWORD bytesRead;
      while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) &&
             bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string frag(buffer);
        {
          std::lock_guard<std::mutex> lock(outputMutex);
          ssOutput << frag;
        }
        if (callback)
          callback(frag);
      }
      threadFinished = true;
    });

    while (!threadFinished) {
      if (stopSignal && stopSignal->load()) {
        TerminateProcess(pi.hProcess, 1);
        {
          std::lock_guard<std::mutex> lock(outputMutex);
          ssOutput << "\n[PROCESS TERMINATED BY USER]\n";
        }
        if (callback)
          callback("\n[TERMINATED]");
        break;
      }

      // Check if process still running
      DWORD waitRes = WaitForSingleObject(pi.hProcess, 50);
      if (waitRes != WAIT_TIMEOUT) {
        // Wait a bit for the reader thread to finish draining the pipe
        int timeout = 0;
        while (!threadFinished && timeout < 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          timeout++;
        }
        GetExitCodeProcess(pi.hProcess, (LPDWORD)&result.exitCode);
        break;
      }
    }

    if (reader.joinable())
      reader.join();
    result.output = ssOutput.str();

    CloseHandle(pi.hThread);
    return result;
  }

  // Fast help scraper for specific commands
  static std::string GetHelp(const std::string &command) {
    if (command.empty())
      return "";

    // We run the command with /? and limit it to first few lines
    // This uses a very fast timeout to ensure no hang
    std::string helpCmd = command + " /?";
    auto res = Execute(helpCmd);

    std::stringstream ss(res.output);
    std::string line;
    std::string summary = "";
    int count = 0;
    while (std::getline(ss, line) && count < 6) {
      if (line.find_first_not_of(" \t\r\n") != std::string::npos) {
        summary += line + "\n";
        count++;
      }
    }
    return summary;
  }
};
