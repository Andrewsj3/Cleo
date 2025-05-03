#pragma once

#include "command.hpp"
#include <flat_map>
#include <functional>
#include <string>

void inputThread();
void backgroundThread(const std::flat_map<std::string, std::function<void(Command&)>>&);
