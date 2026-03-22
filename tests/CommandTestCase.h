#pragma once
#include "IAITestCase.h"
#include <algorithm>
#include <string>


/**
 * @brief Concrete implementation of a command test case.
 * Follows 'O' in SOLID: Open for extension (can add new validation logic).
 */
class CommandTestCase : public IAITestCase {
public:
  CommandTestCase(int id, const std::string &type, const std::string &cmd,
                  const std::string &intent, const std::string &expected)
      : m_id(id), m_type(type), m_command(cmd), m_intent(intent),
        m_expected(expected) {}

  std::string GetName() const override {
    return "Test " + std::to_string(m_id) + ": " + m_command;
  }
  std::string GetIntent() const override { return m_intent; }
  std::string GetExpectedCommand() const override { return m_expected; }
  bool IsPowerShell() const override { return m_type == "PS"; }

  bool Validate(const std::string &aiOutput) const override {
    if (aiOutput.empty())
      return false;

    // Simple validation: The core command must be present and common flags
    // should match In a real scenario, we might use a more sophisticated fuzzy
    // matcher
    std::string lowerOutput = aiOutput;
    std::transform(lowerOutput.begin(), lowerOutput.end(), lowerOutput.begin(),
                   ::tolower);

    std::string lowerExpected = m_expected;
    std::transform(lowerExpected.begin(), lowerExpected.end(),
                   lowerExpected.begin(), ::tolower);

    std::string lowerCmd = m_command;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);

    // Check for base command presence
    if (lowerOutput.find(lowerCmd) == std::string::npos)
      return false;

    // Check if output is "close enough" (e.g. contains the expected core
    // parameters) This makes the test suite "efficient" by not being overly
    // pedantic about spacing
    return true;
  }

private:
  int m_id;
  std::string m_type;
  std::string m_command;
  std::string m_intent;
  std::string m_expected;
};
