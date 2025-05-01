#include "command.hpp"
#include <cassert>
#include <stdexcept>

Command::Command(const std::vector<std::string>& components) {
    assert(components.size() >= 1 && "Command was empty");
    mFunction = components.at(0);
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
    mArguments.erase(mArguments.begin());
}
