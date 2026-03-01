#pragma once
#include <algorithm>
#include <string>
#include <vector>

class CommandFirewall {
public:
  struct BlockResult {
    bool blocked = false;
    std::string reason = "";
  };

  static BlockResult Assess(const std::string &input) {
    BlockResult res;
    if (input.empty())
      return res;

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // 1. Technical Subjects & Objects
    static const std::vector<std::string> techSubjects = {
        "user",       "account", "pass",     "file",     "folder",     "dir",
        "path",       "disk",    "drive",    "process",  "task",       "app",
        "service",    "daemon",  "log",      "event",    "net",        "ip",
        "port",       "dns",     "adapter",  "wifi",     "host",       "ping",
        "sys",        "version", "cpu",      "ram",      "memory",     "health",
        "battery",    "reg",     "registry", "policy",   "gpo",        "group",
        "permission", "acl",     "shell",    "cmd",      "powershell", "pwsh",
        "bash",       "cli",     "script",   "install",  "uninstall",  "update",
        "patch",      "chkdsk",  "sfc",      "terminal", "screen",     "buffer",
        "display",    "window",  "monitor"};

    // 2. Direct CLI Action Indicators
    static const std::vector<std::string> cliActions = {
        "list",   "show",   "get",  "find",   "search", "check",   "scan",
        "create", "make",   "new",  "add",    "set",    "update",  "edit",
        "delete", "remove", "kill", "stop",   "start",  "restart", "move",
        "copy",   "run",    "exec", "launch", "open",   "fix",     "repair",
        "clear",  "cls",    "wipe", "reset",  "exit",   "close"};

    // 3. Direct Whitelist (Bypass entirely)
    static const std::vector<std::string> directWhitelist = {
        "cls", "clear", "ver", "whoami", "ipconfig",
        "dir", "ls",    "pwd", "exit"};

    // 3. Conversational Correction Indicators (Allow users to point out AI
    // mistakes)
    static const std::vector<std::string> correctionTerms = {
        "wrong", "incorrect", "parameter",   "hallucination", "mistake",
        "error", "try again", "not working", "fails",         "fail"};

    // 4. Quick Built-in Check (e.g. 'dir', 'ipconfig')
    // 5. Short Command Bypass
    if (input.length() <= 3) {
      return res; // Allow very short strings like 'cls', 'dir'
    }

    // --- THE HYBRID HEURISTIC ---
    bool isDirectCommand = false;
    for (const auto &cmd : directWhitelist) {
      if (lower == cmd) {
        isDirectCommand = true;
        break;
      }
    }
    if (isDirectCommand)
      return res;

    bool hasTechSubject = false;
    for (const auto &subject : techSubjects) {
      if (lower.find(subject) != std::string::npos) {
        hasTechSubject = true;
        break;
      }
    }

    bool hasCliAction = false;
    for (const auto &action : cliActions) {
      if (lower.find(action) != std::string::npos) {
        hasCliAction = true;
        break;
      }
    }

    bool isCorrection = false;
    for (const auto &term : correctionTerms) {
      if (lower.find(term) != std::string::npos) {
        isCorrection = true;
        break;
      }
    }

    // Pass if it's a correction, OR has a tech subject, OR has a CLI action
    // verb.
    if (!isCorrection && !hasTechSubject && !hasCliAction) {
      res.blocked = true;
      res.reason =
          "I'm sorry, but I can only assist with Windows command-line, system "
          "administration, or technical troubleshooting tasks. Please provide "
          "a request related to system files, network configuration, or "
          "hardware management.";
    }

    return res;
  }
};
