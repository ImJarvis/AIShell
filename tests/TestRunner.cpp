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

    // 3. Drive Info (mountvol is faster and more reliable than wmic)
    {
        auto res = ShellManager::Execute("mountvol");
        ASSERT_EQ(res.exitCode, 0, "mountvol execution");
        bool found = res.output.find("\\") != std::string::npos;
        ASSERT_EQ(found, true, "mountvol output contains mount points");
    }

    // 4. Windows Version (ver)
    {
        auto res = ShellManager::Execute("ver");
        ASSERT_EQ(res.exitCode, 0, "ver execution");
        bool found = res.output.find("Windows") != std::string::npos;
        ASSERT_EQ(found, true, "ver output contains 'Windows'");
    }

    // 5. Logical Disk Info (wmic) - Simplified to just names for speed
    {
        auto res = ShellManager::Execute("wmic logicaldisk get name");
        ASSERT_EQ(res.exitCode, 0, "wmic logicaldisk execution");
        bool found = res.output.find("Name") != std::string::npos;
        ASSERT_EQ(found, true, "wmic output contains 'Name'");
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

    std::cout << "\n========================================" << std::endl;
    std::cout << "TOTAL MODULES PASSED: " << passed << "/4" << std::endl;
    std::cout << "========================================" << std::endl;

    return (passed == 4) ? 0 : 1;
}
