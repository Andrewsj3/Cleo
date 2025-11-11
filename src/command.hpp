#pragma once

#include <cstddef>
#include <string>
#include <vector>
class Command {
public:
    Command(const std::vector<std::string>&);
    Command() = default;

    const std::string& function() const;
    const std::vector<std::string>& arguments() const;
    std::string nextArg();
    std::size_t argCount();

private:
    std::string mFunction{};
    std::vector<std::string> mArguments{};
    void shift();
};
