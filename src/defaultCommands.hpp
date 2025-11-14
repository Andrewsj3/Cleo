#pragma once

#include "command.hpp"
#include <flat_map>
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
    void time(Command&);
    void loop(Command&);
    void repeat(Command&);
    void rename(Command&);
    void del(Command&);
    void playlist(Command&);
    const extern std::flat_map<std::string, std::function<void(Command&)>> commands;
    const extern std::flat_map<std::string, std::string> commandHelp;
    const extern std::vector<std::string> commandList;
} // namespace Cleo
