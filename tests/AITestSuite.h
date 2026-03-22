#pragma once
#include "IAIProvider.h"
#include "IAITestCase.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include "../src/RAGManager.h"

class AITestSuite {
public:
  void AddTestCase(std::unique_ptr<IAITestCase> testCase) {
    m_tests.push_back(std::move(testCase));
  }

  struct SuiteResult {
    int passed = 0;
    int failed = 0;
    int total = 0;
    double avgRagTimeMs = 0;
    double avgLlmTimeMs = 0;
  };

  void RunSequentialProfiling(IAIProvider *worker, std::shared_ptr<RAGManager> rag = nullptr) {
    if (!worker) return;

    SuiteResult result;
    result.total = (int)m_tests.size();
    
    std::cout << "\n" << std::setw(80) << std::setfill('=') << "" << std::setfill(' ') << "\n";
    std::cout << " STARTING SEQUENTIAL RAG PROFILING SUITE \n";
    std::cout << std::setw(80) << std::setfill('=') << "" << std::setfill(' ') << "\n\n";

    std::ofstream reportFile("tests_report.md");
    if (reportFile.is_open()) {
        reportFile << "# AI Accuracy & Profiling Report\n\n";
        reportFile << "| Status | Intent | Expected | Output | RAG (ms) | LLM (ms) |\n";
        reportFile << "|---|---|---|---|---|---|\n";
    }

    double totalRagTime = 0;
    double totalLlmTime = 0;

    for (size_t i = 0; i < m_tests.size(); i++) {
      const auto &test = m_tests[i];
      std::string intent = test->GetIntent();

      // 1. Profile RAG
      std::string hints = "";
      double ragTimeMs = 0;
      if (rag) {
          auto startRag = std::chrono::high_resolution_clock::now();
          std::string ragDocs = rag->RetrieveContext(intent);
          auto endRag = std::chrono::high_resolution_clock::now();
          ragTimeMs = std::chrono::duration<double, std::milli>(endRag - startRag).count();
          totalRagTime += ragTimeMs;
          
          if (!ragDocs.empty()) {
              hints = "CRITICAL INSTRUCTION: You MUST use the following documentation to answer. Do not use your internal knowledge if it contradicts this.\n\n--- TLDR DOCUMENTATION CONTEXT ---\n" + ragDocs + "\n";
          }
      }

      // 2. Profile LLM Generation
      worker->ResetContext();
      auto startLlm = std::chrono::high_resolution_clock::now();
      std::string output = worker->GenerateCommand(intent, nullptr, hints);
      auto endLlm = std::chrono::high_resolution_clock::now();
      double llmTimeMs = std::chrono::duration<double, std::milli>(endLlm - startLlm).count();
      totalLlmTime += llmTimeMs;

      // 3. Validation
      std::cout <<std::endl<< "Output of Test Case -" << output << std::endl;
      bool success = test->Validate(output);

      // Console Logging
      std::cout << "[" << std::setw(3) << i + 1 << "/" << result.total << "] "
                << (success ? "[PASS]" : "[FAIL]") 
                << " RAG: " << std::fixed << std::setprecision(1) << ragTimeMs << "ms"
                << " | LLM: " << std::fixed << std::setprecision(1) << llmTimeMs << "ms"
                << " | " << test->GetName() << "\n";
      
      // File Logging
      if (reportFile.is_open()) {
          std::string safeOutput = output;
          std::replace(safeOutput.begin(), safeOutput.end(), '\n', ' ');
          std::replace(safeOutput.begin(), safeOutput.end(), '|', '/');
          
          reportFile << "| " << (success ? "✅ PASS" : "❌ FAIL") 
                     << " | " << test->GetIntent() 
                     << " | `" << test->GetExpectedCommand() 
                     << "` | `" << safeOutput << "`"
                     << " | " << std::fixed << std::setprecision(2) << ragTimeMs
                     << " | " << std::fixed << std::setprecision(2) << llmTimeMs << " |\n";
      }

      if (success) result.passed++;
      else result.failed++;
    }

    result.avgRagTimeMs = result.total > 0 ? (totalRagTime / result.total) : 0;
    result.avgLlmTimeMs = result.total > 0 ? (totalLlmTime / result.total) : 0;

    std::cout << "\n" << std::setw(80) << std::setfill('=') << "" << std::setfill(' ') << "\n";
    std::cout << "FINAL ACCURACY: " << (float)result.passed / result.total * 100 << "%\n";
    std::cout << "Passed: " << result.passed << " | Failed: " << result.failed << "\n";
    std::cout << "Avg RAG Time: " << result.avgRagTimeMs << "ms | Avg LLM Time: " << result.avgLlmTimeMs << "ms\n";
    std::cout << std::setw(80) << std::setfill('=') << "" << std::setfill(' ') << "\n";
              
    if (reportFile.is_open()) {
        reportFile << "\n## Summary\n"
                   << "**Accuracy**: " << (float)result.passed / result.total * 100 << "%\n"
                   << "**Passed**: " << result.passed << "\n"
                   << "**Failed**: " << result.failed << "\n"
                   << "**Average RAG Time**: " << std::fixed << std::setprecision(1) << result.avgRagTimeMs << " ms\n"
                   << "**Average LLM Time**: " << std::fixed << std::setprecision(1) << result.avgLlmTimeMs << " ms\n";
        reportFile.close();
        std::cout << "Detailed report saved to tests_report.md\n";
    }
  }

private:
  std::vector<std::unique_ptr<IAITestCase>> m_tests;
};
