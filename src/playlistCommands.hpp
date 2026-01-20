#pragma once

#include "command.hpp"
#include <flat_map>
namespace Cleo::Playlist {
    void load(Command&);
    void play(Command&);
    void add(Command&);
    void save(Command&);
    void shuffle(Command&);
    void status(Command&);
    void find(Command&);
    void next(Command&);
    void previous(Command&);
    void loop(Command&);
    void clear(Command&);
    void remove(Command&);
    void del(Command&);
    void skip(Command&);
    const extern std::flat_map<std::string, std::function<void(Command&)>> commands;
    const extern std::flat_map<std::string, std::string> commandHelp;
    const extern std::vector<std::string> commandList;
} // namespace Cleo::Playlist
