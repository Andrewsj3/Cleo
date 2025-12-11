#pragma once

#include "command.hpp"
#include <flat_map>
void inputThread();
void backgroundThread();
void parseCmd(Command& cmd,
              const std::flat_map<std::string, std::function<void(Command&)>>& programCommands);
std::vector<Command> parseString(std::string_view input);
void executeCmds(const std::vector<Command>& commands);
