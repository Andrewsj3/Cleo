#pragma once

#include "command.hpp"
#include <string>
#include <string_view>
#include <vector>
std::string join(const std::vector<std::string>&, std::string_view);

namespace Cleo {
    void help(Command&);
    void play(Command&);
    void list(Command&);
    void stop(Command&);
    void volume(Command&);
    void pause(Command&);
    void exit(Command&);
} // namespace Cleo
