#include "autocomplete.hpp"
#include "command.hpp"
#include "threads.hpp"
#include <flat_map>
#include <print>
#include <string>
#if defined(__unix)
#include "readline/history.h"
#include "readline/readline.h"
#elif defined(WIN32)
#error "Unix based only, sorry"
#endif

using commandMap = std::flat_map<std::string, std::function<void(Command&)>>;

std::vector<Command> parseString(std::string_view input) {
    bool isQuoted{false};
    std::vector<Command> commands{};
    std::vector<std::string> thisCommand{};
    std::stringstream current{};
    for (const auto& c : input) {
        if (c == '\"') {
            isQuoted ^= true;
            continue;
        } else if (isQuoted) {
            current << c;
            continue;
        } else if (c == ' ') {
            if (!current.str().empty()) {
                thisCommand.push_back(current.str());
                current.str("");
            }
        } else if (c == ';') {
            if (!current.str().empty()) {
                thisCommand.push_back(current.str());
                current.str("");
                commands.emplace_back(thisCommand);
                thisCommand.clear();
            }
        } else {
            current << c;
        }
    }
    if (!current.str().empty()) {
        thisCommand.push_back(current.str());
        commands.emplace_back(thisCommand);
    }
    return commands;
}

bool checkInput() { return !Threads::userInput.empty(); }

void parseCmd(Command& cmd, const commandMap& programCommands) {
    std::string match{};
    if (programCommands.contains(cmd.function())) {
        programCommands.at(cmd.function())(cmd);
    } else {
        switch (autocomplete(programCommands.keys(), cmd.function(), match)) {
        case Match::NoMatch:
            std::println("Command '{}' not found", cmd.function());
            break;
        case Match::ExactMatch:
            programCommands.at(match)(cmd);
            return;
        case Match::MultipleMatch:
            std::println("Multiple possible commands found, try refining your search");
            break;
        }
    }
}

bool existsInHistory(HIST_ENTRY** history, const char* str) {
    if (history == NULL) {
        return false;
    }

    for (size_t i{0}; history[i] != NULL; ++i) {
        if (strcmp(history[i]->line, str) == 0) {
            return true;
        }
    }
    return false;
}

void inputThread() {
    std::string input{};
    while (true) {
        if (!Threads::running)
            return;
        if (!Threads::readyForInput)
            continue;
        const char* input = readline("> ");
        if (input == NULL /*in case of EOF*/) {
            Threads::running = false;
            return;
        }
        Threads::userInput = input;
        if (Threads::userInput.length() > 0) {
            if (!existsInHistory(history_list(), Threads::userInput.c_str()))
                add_history(Threads::userInput.c_str());
        }
    }
}

void backgroundThread(const commandMap& programCommands) {
    std::vector<Command> commands{};
    while (true) {
        if (!Threads::running)
            return;
        if (!checkInput())
            continue;
        Threads::readyForInput = false;
        commands = parseString(Threads::userInput);
        for (auto& cmd : commands) {
            parseCmd(cmd, programCommands);
        }
        Threads::userInput.clear();
        Threads::readyForInput = true;
    }
}
