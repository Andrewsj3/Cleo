#include "autocomplete.hpp"
#include "command.hpp"
#include "music.hpp"
#include "threads.hpp"
#include "userCommands.hpp"
#include <SFML/Audio/Music.hpp>
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
static const commandMap programCommands{{"exit", Cleo::exit},   {"list", Cleo::list},
                                        {"pause", Cleo::pause}, {"play", Cleo::play},
                                        {"stop", Cleo::stop},   {"volume", Cleo::volume},
                                        {"help", Cleo::help},   {"time", Cleo::time},
                                        {"loop", Cleo::loop},   {"repeat", Cleo::repeat}};

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
        MusicMatch match{autocomplete(programCommands.keys(), cmd.function())};
        switch (match.matchType) {
        case Match::NoMatch:
            std::println("Command '{}' not found", cmd.function());
            break;
        case Match::ExactMatch:
            programCommands.at(match.exactMatch())(cmd);
            return;
        case Match::MultipleMatch:
            std::println("Multiple possible commands found, could be one of {}",
                         join(matches, ", "));
            break;
        }
    }
}

void executeCmds(const std::vector<Command>& commands) {
    for (auto cmd : commands) {
        if (Threads::helpMode) {
            Cleo::help(cmd);
        } else {
            parseCmd(cmd, programCommands);
        }
        // help on its own sends us into help mode, any subsequent commands should be executed
        // as if we are in help mode unless they return us to normal mode. Previously, commands
        // were executed all in normal mode or all in help mode
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

bool shouldRepeat() {
    if (Music::music.isLooping()) {
        return false;
    }
    long offsetMillis{Music::music.getPlayingOffset().asMilliseconds()};
    long durationMillis{Music::music.getDuration().asMilliseconds()};
    if (offsetMillis == 0) {
        return false;
    }
    if (durationMillis - offsetMillis <= 20) {
        if (Music::repeats > 0) {
            return true;
        }
        Music::curSong = "";
    }
    return false;
}

void inputThread() {
    using namespace std::chrono_literals;
    std::string input{};
    while (true) {
        std::this_thread::sleep_for(
            10ms); // Ensure prompt only appears after any commands have finished running
        if (!Threads::running) [[unlikely]]
            return;
        if (!Threads::readyForInput) [[unlikely]]
            continue;
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
        if (shouldRepeat()) {
            --Music::repeats;
            Music::music.play();
        }
        if (!Threads::running) [[unlikely]]
            return;
        if (!checkInput()) [[likely]] {
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
