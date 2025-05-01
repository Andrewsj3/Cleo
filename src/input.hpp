#pragma once

#include "command.hpp"
#include <functional>
#include <map>
#include <string>

void inputThread();
void backgroundThread(const std::map<std::string, std::function<void(Command&)>>&);
