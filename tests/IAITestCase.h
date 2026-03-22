#pragma once
#include <string>

/**
 * @brief Interface for a single test scenario.
 * Follows the Single Responsibility Principle.
 */
class IAITestCase {
public:
  virtual ~IAITestCase() = default;

  virtual std::string GetName() const = 0;
  virtual std::string GetIntent() const = 0;
  virtual std::string GetExpectedCommand() const = 0;
  virtual bool IsPowerShell() const = 0;

  // Validates if the output from the AI matches the expected outcome
  virtual bool Validate(const std::string &aiOutput) const = 0;
};
