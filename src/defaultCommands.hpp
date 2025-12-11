#pragma once

#include "command.hpp"
#include <flat_map>
#include <string>
#include <string_view>
#include <vector>
std::string join(const std::vector<std::string>& vec, std::string_view delim);
std::string numAsTimestamp(int time);
std::string stem(std::string_view filename);
std::vector<std::string> transformStem(const std::vector<std::string>& input);
void findHelp(const std::flat_map<std::string, std::string>& domain, const std::string& topic);

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
    void seek(Command&);
    void forward(Command&);
    void rewind(Command&);
    void find(Command&);
    void setMusicDir(Command&);
    void setPlaylistDir(Command&);
    void setPrompt(Command&);
    void run(Command&);
    const extern std::flat_map<std::string, std::function<void(Command&)>> commands;
    const extern std::flat_map<std::string, std::string> commandHelp;
    const extern std::vector<std::string> commandList;
} // namespace Cleo
