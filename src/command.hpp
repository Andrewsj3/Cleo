#pragma once

#include <string>
#include <vector>
class Command {
public:
    Command(const std::vector<std::string>&);
    Command() = default;

    const std::string& function() const;
    const std::vector<std::string>& arguments() const;
    void shift();

private:
    std::string mFunction{};
    std::vector<std::string> mArguments{};
};
