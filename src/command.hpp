#pragma once

#include <string>
#include <vector>
class Command {
public:
    Command(const std::vector<std::string>& components);
    Command(std::initializer_list<std::string> components);
    Command(std::string_view cmd, const std::vector<std::string>& args);
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
