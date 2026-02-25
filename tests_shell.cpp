#include <iostream>
#include <string>
#include <vector>
#include "src/ShellManager.h"

// Simple assertion macro
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "[FAIL] " << message << std::endl; \
        return false; \
    } else { \
        std::cout << "[PASS] " << message << std::endl; \
    }

bool TestEchoCommand() {
    auto result = ShellManager::Execute("echo Hello World");
    ASSERT_TRUE(result.exitCode == 0, "Echo command should have exit code 0");
    // _popen might include newline
    ASSERT_TRUE(result.output.find("Hello World") != std::string::npos, "Output should contain 'Hello World'");
    return true;
}

bool TestInvalidCommand() {
    auto result = ShellManager::Execute("non_existent_command_12345");
    // Windows usually returns 1 for command not found in _popen shell
    ASSERT_TRUE(result.exitCode != 0, "Invalid command should have non-zero exit code");
    return true;
}

bool TestDirCommand() {
    auto result = ShellManager::Execute("dir");
    ASSERT_TRUE(result.exitCode == 0, "Dir command should succeed");
    ASSERT_TRUE(!result.output.empty(), "Dir output should not be empty");
    return true;
}

int main() {
    std::cout << "Starting ShellManager Test Suite..." << std::endl;
    std::cout << "------------------------------------" << std::endl;

    int passed = 0;
    int total = 3;

    if (TestEchoCommand()) passed++;
    if (TestInvalidCommand()) passed++;
    if (TestDirCommand()) passed++;

    std::cout << "------------------------------------" << std::endl;
    std::cout << "Tests Passed: " << passed << "/" << total << std::endl;

    return (passed == total) ? 0 : 1;
}
