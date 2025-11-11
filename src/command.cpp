#include "command.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>

void lower(std::string& str) {
    std::transform(str.cbegin(), str.cend(), str.begin(),
                   [](char c) { return std::tolower(c); });
}

Command::Command(const std::vector<std::string>& components) {
    assert(components.size() >= 1 && "Command was empty");
    mFunction = components.at(0);
    lower(mFunction);
    if (components.size() > 1) {
        mArguments = std::vector<std::string>(components.begin() + 1, components.end());
    }
}
const std::string& Command::function() const { return mFunction; }
const std::vector<std::string>& Command::arguments() const { return mArguments; }

void Command::shift() {
    if (mArguments.empty()) {
        throw std::out_of_range(
            "Cannot shift command because there are no remaining arguments");
    }
    mFunction = mArguments.front();
    lower(mFunction);
    mArguments.erase(mArguments.begin());
}

std::string Command::nextArg() {
    std::string arg{mArguments.front()};
    shift();
    return arg;
}

std::size_t Command::argCount() { return mArguments.size(); }
