#include "autocomplete.hpp"
#include "command.hpp"
#include "threads.hpp"
#include "userCommands.hpp"
#include <cstring>
#include <flat_map>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#if defined(__unix)
#include "readline/history.h"
#include "readline/readline.h"
#elif defined(WIN32)
#error "Unix based only, sorry"
#endif

using commandMap = std::flat_map<std::string, std::function<void(Command&)>>;
static const commandMap programCommands{
    {"exit", Cleo::exit}, {"list", Cleo::list}, {"pause", Cleo::pause},
    {"play", Cleo::play}, {"stop", Cleo::stop}, {"volume", Cleo::volume},
    {"help", Cleo::help},
};

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
	std::vector<std::string> matches{};
    if (programCommands.contains(cmd.function())) {
        programCommands.at(cmd.function())(cmd);
    } else {
        switch (autocomplete(programCommands.keys(), cmd.function(), match, matches)) {
        case Match::NoMatch:
            std::println("Command '{}' not found", cmd.function());
            break;
        case Match::ExactMatch:
            programCommands.at(match)(cmd);
            return;
        case Match::MultipleMatch:
            std::println("Multiple possible commands found, could be one of {}", join(matches, ", "));
            break;
        }
    }
}

void executeCmds(const std::vector<Command>& commands) {
    if (!Threads::helpMode) {
        for (auto cmd : commands) {
            parseCmd(cmd, programCommands);
        }
    } else {
        for (auto cmd : commands) {
            Cleo::help(cmd);
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
    using namespace std::chrono_literals;
    std::string input{};
    while (true) {
        if (!Threads::running)
            return;
        if (!Threads::readyForInput)
            continue;
        std::this_thread::sleep_for(10ms); // Ensure prompt only appears after any commands have
                                           // finished executing
        const char* prompt = Threads::helpMode ? "?> " : "> ";
        const char* input = readline(prompt);
        if (input == NULL /*in case of EOF*/) {
            if (Threads::helpMode) /*Exit out of help mode*/ {
                Threads::helpMode = false;
                continue;
            } else {
                Threads::running = false;
                return;
            }
        } else if (strcmp(input, "exit") == 0 && Threads::helpMode) {
            Threads::helpMode = false;
            continue;
        }
        Threads::userInput = input;
        if (Threads::userInput.length() > 0) {
            if (!existsInHistory(history_list(), Threads::userInput.c_str()))
                add_history(Threads::userInput.c_str());
        }
    }
}

void backgroundThread() {
    using namespace std::chrono_literals;
    std::vector<Command> commands{};
    while (true) {
        if (!Threads::running)
            return;
        if (!checkInput()) {
            std::this_thread::sleep_for(10ms); // otherwise CPU goes brrrrrrr
            continue;
        }
        Threads::readyForInput = false;
        commands = parseString(Threads::userInput);
        executeCmds(commands);
        Threads::userInput.clear();
        Threads::readyForInput = true;
    }
}
