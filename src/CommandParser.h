#pragma once
#include <string>

struct ParsedCommand {
    std::string command;
    std::string explanation;
    bool success = false;
};

class CommandParser {
public:
    static ParsedCommand Parse(const std::string& fullResponse);
};
