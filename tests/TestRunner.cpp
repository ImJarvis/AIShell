#include "../src/CommandParser.h"
#include "../src/LlamaManager.h"
#include "../src/ShellManager.h"
#include "../src/ShellManager.h"
#include "../src/JsonKnowledgeBase.h"
#include "../src/RAGManager.h"
#include "AITestSuite.h"
#include "CommandData.h"
#include "CommandTestCase.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../src/WebSearchManager.h"

// Simple assertion macro
#define ASSERT_EQ(val1, val2, message)                                         \
  if ((val1) != (val2)) {                                                      \
    std::cerr << " [FAIL] " << message << std::endl;                           \
    std::cerr << "        Expected : '" << (val2) << "'" << std::endl;         \
    std::cerr << "        Got      : '" << (val1) << "'" << std::endl;         \
    return false;                                                              \
  }

bool TestWebSearch() {
  std::cout << "\n--- Testing Web Search Module (Brave HTTP) ---" << std::endl;
  std::string result = WebSearchManager::SearchBrave("how to fix positional parameter cannot be found powershell");
  if (result.empty() || result.length() < 20) {
      std::cerr << " [FAIL] WebSearchManager returned empty or invalid snippets." << std::endl;
      return false;
  }
  std::cout << " [PASS] WebSearchManager (Found " << result.length() << " chars of snippets)" << std::endl;
  return true;
}

bool TestShellExecution() {
  std::cout << "\n--- Testing Shell Execution ---" << std::endl;
  auto result = ShellManager::Execute("echo HollowShellTest");
  ASSERT_EQ(result.exitCode, 0, "Echo command exit code");
  bool found = result.output.find("HollowShellTest") != std::string::npos;
  ASSERT_EQ(found, true, "Echo output content");
  std::cout << " [PASS] Shell Execution" << std::endl;
  return true;
}

bool TestCommandParsing() {
  std::cout << "\n--- Testing Command Parsing ---" << std::endl;

  // Test 1: Standard tags
  {
    std::string input = "[CMD] dir /w [WHY] Lists files in wide format";
    auto pc = CommandParser::Parse(input);
    ASSERT_EQ(pc.command, "dir /w", "Standard [CMD] tag extraction");
  }

  // Test 2: Markdown code blocks
  {
    std::string input =
        "You should run this:\n```\ncd src\n```\nIt moves to source.";
    auto pc = CommandParser::Parse(input);
    ASSERT_EQ(pc.command, "cd src", "Markdown code block extraction");
  }

  // Test 3: Mixed content with backticks
  {
    std::string input = "Run `ipconfig /all` to see network info.";
    auto pc = CommandParser::Parse(input);
    ASSERT_EQ(pc.command, "ipconfig /all", "Single backtick extraction");
  }

  // Test 4: JSON structured extraction
  {
    std::string input = "{\"cmd\": \"netsh wlan show profile\", \"why\": "
                        "\"Lists saved Wi-Fi profiles.\"}";
    auto pc = CommandParser::Parse(input);
    ASSERT_EQ(pc.command, "netsh wlan show profile", "JSON extraction");
  }

  std::cout << " [PASS] Command Parsing" << std::endl;
  return true;
}

int main(int argc, char **argv) {
  std::cout << "========================================" << std::endl;
  std::cout << "   AI HOLLOW SHELL - COMPREHENSIVE TESTER " << std::endl;
  std::cout << "========================================" << std::endl;

  // 1. Module Tests
  bool modulesOk = true;
  modulesOk &= TestWebSearch();
  modulesOk &= TestShellExecution();
  modulesOk &= TestCommandParsing();

  if (!modulesOk) {
    std::cerr << "\n[CRITICAL] Core modules failed. Aborting full suite."
              << std::endl;
    return 1;
  }

  // 2. AI Accuracy Suite (The 100 Commands)
  std::string modelPath = "";
  if (argc > 1) {
    modelPath = argv[1];
  } else {
    // Attempt default path
    modelPath = "models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf";
  }

  std::cout << "\nStarting AI Accuracy Suite with "
            << (modelPath.empty() ? "Mock (Simulated)" : modelPath) << "..."
            << std::endl;

  // Initialize the Suite
  AITestSuite suite;
  for (const auto &data : c_CommandDatabase) {
    suite.AddTestCase(std::make_unique<CommandTestCase>(
        data.id, data.type, data.command, data.intent, data.expected));
  }

  // Run tests if model is available
  if (!modelPath.empty() &&
      GetFileAttributesA(modelPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
    try {
      // 3. Initialize RAG
      auto kb = std::make_shared<JsonKnowledgeBase>();
      
      // Hunt for tldr_database.json moving up the tree
      if (!kb->Load("tldr_database.json")) {
          if (!kb->Load("../tldr_database.json")) {
              if (!kb->Load("../../tldr_database.json")) {
                  if (!kb->Load("../../../tldr_database.json")) {
                      if (!kb->Load("../../../../tldr_database.json")) {
                          kb->Load("../../../../../tldr_database.json");
                      }
                  }
              }
          }
      }
      auto rag = std::make_shared<RAGManager>(kb);

      std::cout << "Initializing single sequential worker for profiling..." << std::endl;
      
      LlamaManager worker(modelPath, modelPath);
      
      suite.RunSequentialProfiling(&worker, rag);
    } catch (...) {
      std::cerr << "Failed to initialize AI worker for tests."
                << std::endl;
    }
  } else {
    std::cout << "[SKIP] Real AI tests skipped (Model not found at "
              << modelPath << ")" << std::endl;
    std::cout
        << "Use: ShellTests.exe <path_to_gguf> to run full accuracy suite."
        << std::endl;
  }

  std::cout << "\n========================================" << std::endl;
  return 0;
}
