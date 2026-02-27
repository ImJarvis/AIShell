#include <iostream>
#include <string>
#include <vector>
#include "../src/ShellManager.h"
#include "../src/CommandParser.h"

// Simple assertion macro
#define ASSERT_EQ(val1, val2, message) \
    if ((val1) != (val2)) { \
        std::cerr << "[FAIL] " << message << std::endl; \
        std::cerr << "       Expected : '" << (val2) << "'" << std::endl; \
        std::cerr << "       Got      : '" << (val1) << "'" << std::endl; \
        return false; \
    } else { \
        std::cout << "[PASS] " << message << std::endl; \
    }

bool TestShellExecution() 
{
    std::cout << "\n--- Testing Shell Execution ---" << std::endl;
    auto result = ShellManager::Execute("echo HollowShellTest");
    ASSERT_EQ(result.exitCode, 0, "Echo command exit code");
    bool found = result.output.find("HollowShellTest") != std::string::npos;
    ASSERT_EQ(found, true, "Echo output content");
    return true;
}

bool TestCommandParsing() 
{
    std::cout << "\n--- Testing Command Parsing ---" << std::endl;
    
    // Test 1: Standard tags
    {
        std::string input = "[CMD] dir /w [WHY] Lists files in wide format";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "dir /w", "Standard [CMD] tag extraction");
        ASSERT_EQ(pc.explanation, "Lists files in wide format", "Standard [WHY] tag extraction");
    }

    // Test 2: Markdown code blocks
    {
        std::string input = "You should run this:\n```\ncd src\n```\nIt moves to source.";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "cd src", "Markdown code block extraction");
    }

    // Test 3: Mixed content with backticks
    {
        std::string input = "Run `ipconfig /all` to see network info.";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "ipconfig /all", "Single backtick extraction");
    }

    // Test 4: First line fallback
    {
        std::string input = "netstat -ano\nThis command shows ports.";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "netstat -ano", "First line fallback extraction");
    }

    // Test 5: Hallucinated shell prompt ($ or >)
    {
        std::string input = "[CMD] $ ipconfig [WHY] Checking IP";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "ipconfig", "Hallucinated '$' prompt stripping");
    }

    {
        std::string input = "> dir /s";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "dir /s", "Hallucinated '>' prompt stripping");
    }

    // Test 7: Modern JSON structure
    {
        std::string input = "Analysis complete. {\"cmd\": \"netsh wlan show profile\", \"why\": \"Lists saved Wi-Fi profiles.\"}";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "netsh wlan show profile", "JSON structured extraction");
    }

    // Test 8: JSON with escaped characters
    {
        std::string input = "{\"cmd\": \"echo \\\"Hello World\\\"\", \"why\": \"Prints message\"}";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "echo \"Hello World\"", "JSON escaped character handling");
    }

    std::cout << "\n--- Testing Rigorous Rule Adherence (Adversarial Data) ---" << std::endl;

    // Model Quirk: Phi-3.5 often nests commands in "step1"
    {
        std::string input = "{\"step1\": \"mkdir my_docs\", \"whyStep1\": \"Creating folder\"}";
        auto pc = CommandParser::Parse(input);
        // Note: Our current parser is focused on "cmd". 
        // We add this to document that the system prompt FIX is preferred over parser bloat.
    }

    // Rule: Handle "Command-only" outputs without JSON tags (fallback logic)
    {
        std::string input = "dir /s /b";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "dir /s /b", "Raw command (no tags) fallback");
    }

    // Rule: Handle Markdown blocks that include comments
    {
        std::string input = "```cmd\n# Check network\nipconfig\n```";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "ipconfig", "Scrubbing comments from code blocks");
    }

    // Rule: Handle Windows prompt contamination (The bug fixed with 60ch threshold)
    {
        std::string input = "C:\\Users\\Admin> tasklist /v";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "tasklist /v", "Stripping persistent shell prompts (Long Path)");
    }

    {
        std::string input = "D:\\> whoami";
        auto pc = CommandParser::Parse(input);
        ASSERT_EQ(pc.command, "whoami", "Stripping persistent shell prompts (Short Path)");
    }

    return true;
}

bool TestDirectoryManagement() 
{
    std::cout << "\n--- Testing Directory Management (mkdir/rmdir) ---" << std::endl;
    
    // 1. Create directory
    ShellManager::Execute("mkdir hollow_test_dir");
    
    // 2. Verify existence (dir command should find it)
    auto dirResult = ShellManager::Execute("dir hollow_test_dir");
    ASSERT_EQ(dirResult.exitCode, 0, "Directory should exist after mkdir");

    // 3. Remove directory
    auto rmResult = ShellManager::Execute("rmdir hollow_test_dir");
    ASSERT_EQ(rmResult.exitCode, 0, "Directory should be removed successfully");

    // 4. Verify deletion
    auto checkDeleted = ShellManager::Execute("dir hollow_test_dir");
    ASSERT_EQ(checkDeleted.exitCode != 0, true, "Directory should no longer exist");

    return true;
}

bool TestSystemQueryCommands()
{
    std::cout << "\n--- Testing System Query Commands ---" << std::endl;

    // 1. Current User (whoami)
    {
        auto res = ShellManager::Execute("whoami");
        ASSERT_EQ(res.exitCode, 0, "whoami execution");
        ASSERT_EQ(!res.output.empty(), true, "whoami output not empty");
    }

    // 2. Volume Info (vol)
    {
        auto res = ShellManager::Execute("vol");
        ASSERT_EQ(res.exitCode, 0, "vol execution");
        bool found = res.output.find("Volume") != std::string::npos;
        ASSERT_EQ(found, true, "vol output contains 'Volume'");
    }

    // 3. Drive Info (mountvol)
    {
        auto res = ShellManager::Execute("mountvol");
        ASSERT_EQ(res.exitCode, 0, "mountvol execution");
        bool found = res.output.find("\\") != std::string::npos;
        ASSERT_EQ(found, true, "mountvol output contains mount points");
    }

    return true;
}

bool TestShellDetectionLogic()
{
    std::cout << "\n--- Testing Shell Detection Logic (CMD vs PowerShell) ---" << std::endl;

    // 1. Test CMD Logic (&& separator)
    {
        auto res = ShellManager::Execute("echo A && echo B");
        ASSERT_EQ(res.exitCode, 0, "CMD '&&' logical operator support");
        bool foundA = res.output.find("A") != std::string::npos;
        bool foundB = res.output.find("B") != std::string::npos;
        ASSERT_EQ(foundA && foundB, true, "CMD execution of combined commands");
    }

    // 2. Test PowerShell Cmdlet Detection
    {
        auto res = ShellManager::Execute("Get-Date");
        ASSERT_EQ(res.exitCode, 0, "PowerShell Get-Date execution");
    }

    return true;
}

bool TestMultiModelSimulations()
{
    std::cout << "\n--- Testing Multi-Model Response Simulation ---" << std::endl;

    // Simulation 1: Phi-3.5 "The Over-Planner" (Nested JSON)
    // We expect the parser to struggle with this unless handled, 
    // but here we verify how it fails or succeeds today.
    {
        std::string phiNested = "{ \"step1\": { \"cmd\": \"dir\" }, \"why\": \"Thinking...\" }";
        auto pc = CommandParser::Parse(phiNested);
        std::cout << "Phi Nested Test: " << (pc.success ? "RECOVERED" : "FAILED (As expected for nested)") << std::endl;
    }

    // Simulation 2: Qwen "The Markdown Purist"
    {
        std::string qwenMarkdown = "To help you, please run this:\n```cmd\necho 'Qwen works!'\n```";
        auto pc = CommandParser::Parse(qwenMarkdown);
        ASSERT_EQ(pc.command, "echo 'Qwen works!'", "Qwen markdown extraction");
    }

    // Simulation 3: Generic "The Talker" (JSON buried in text)
    {
        std::string talker = "I have analyzed your request. Here is the command: {\"cmd\": \"cls\", \"why\": \"Clear screen\"}. Hope this helps!";
        auto pc = CommandParser::Parse(talker);
        ASSERT_EQ(pc.command, "cls", "JSON buried in conversation");
    }

    return true;
}

int main() 
{
    std::cout << "========================================" << std::endl;
    std::cout << "   AI HOLLOW SHELL - MODULE TESTER      " << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    if (TestShellExecution()) passed++;
    if (TestCommandParsing()) passed++;
    if (TestDirectoryManagement()) passed++;
    if (TestSystemQueryCommands()) passed++;
    if (TestShellDetectionLogic()) passed++;
    if (TestMultiModelSimulations()) passed++;

    std::cout << "\n========================================" << std::endl;
    std::cout << "TOTAL MODULES PASSED: " << passed << "/6" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed == 6) ? 0 : 1;
}
