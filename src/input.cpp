#include "input.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "defaultCommands.hpp"
#include "music.hpp"
#include "playlistCommands.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <cstddef>
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

using CommandMap = std::flat_map<std::string, std::function<void(Command&)>>;

std::vector<Command> parseString(std::string_view input) {
    bool isQuoted{false};
    std::vector<Command> commands{};
    std::vector<std::string> thisCommand{};
    std::stringstream current{};
    for (const auto& c : input) {
        if (c == '\"' || c == '\'') {
            isQuoted ^= true;
            continue;
        } else if (isQuoted) {
            // process characters verbatim
            current << c;
            continue;
        } else if (c == ' ') {
            if (!current.str().empty()) {
                thisCommand.push_back(current.str());
                current.str("");
            }
        } else if (c == ';') {
            // Allow multiple commands on one line
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

void parseCmd(Command& cmd, const CommandMap& commands) {
    std::string match{};
    if (commands.contains(cmd.function())) {
        commands.at(cmd.function())(cmd);
    } else {
        MusicMatch match{autocomplete(commands.keys(), cmd.function())};
        switch (match.matchType) {
            case Match::NoMatch:
                std::println("Command '{}' not found.", cmd.function());
                break;
            case Match::ExactMatch:
                commands.at(match.exactMatch())(cmd);
                return;
            case Match::MultipleMatch:
                std::println("Multiple possible commands found, could be one of {}.",
                             join(match.matches, ", "));
                break;
        }
    }
}

void executeCmds(const std::vector<Command>& commands) {
    for (auto cmd : commands) {
        if (Threads::helpMode) {
            // Note that this can change between commands, so some commands may be executed in
            // help mode and others normally
            Cleo::help(cmd);
        } else {
            parseCmd(cmd, Cleo::commands);
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

bool shouldRepeat() {
    if (Music::music.isLooping()) {
        return false;
    }
    int offsetMillis{Music::music.getPlayingOffset().asMilliseconds()};
    int durationMillis{Music::music.getDuration().asMilliseconds()};
    if (Music::music.getStatus() != sf::Music::Status::Playing) {
        return false;
    }
    if (durationMillis - offsetMillis <= 20) {
        // Here we determine if the song is about to end and if we should repeat.
        // Due to the rate at which this function gets called, a value lower than 20 may fail
        // sometimes and the song may not repeat.
        if (Music::repeats > 0) {
            return true;
        }
        Music::curSong = "";
    }
    return false;
}

bool shouldAdvance() {
    if (Music::playlistIdx == 0 || !Music::inPlaylistMode ||
        Music::music.getStatus() == sf::Music::Status::Playing) {
        return false;
    } else if (Music::playlistIdx < Music::curPlaylist.size()) {
        return true;
    } else {
        Music::inPlaylistMode = false;
        return false;
    }
}

void inputThread() {
    using namespace std::chrono_literals;
    std::string input{};
    while (Threads::running) {
        if (!Threads::readyForInput) [[unlikely]]
            continue;
        const char* prompt = Threads::helpMode ? "?> " : "> ";
        const char* input = readline(prompt);
        if (input == NULL /*in case of EOF*/) {
            if (Threads::helpMode) {
                Threads::helpMode = false;
                continue;
            } else {
                Threads::running = false;
                return;
            }
        }
        Threads::readyForInput = false;
        // Prevent prompt from showing up until commands have finished executing
        Threads::userInput = std::move(input);
        std::free((void*)input);
        if (Threads::userInput.length() > 0) {
            if (!existsInHistory(history_list(), Threads::userInput.c_str()))
                add_history(Threads::userInput.c_str());
        }
    }
}

void backgroundThread() {
    using namespace std::chrono_literals;
    std::vector<Command> commands{};
    Command _;
    while (Threads::running) {
        if (shouldRepeat()) {
            --Music::repeats;
            Music::music.play();
        }
        if (shouldAdvance()) {
            Cleo::Playlist::play(_);
            // This function doesn't need arguments, but the signature is required, so we pass
            // an empty command to satisfy it
        }
        if (!checkInput()) [[likely]] {
            std::this_thread::sleep_for(10ms);
            continue;
        }
        commands = parseString(Threads::userInput);
        executeCmds(commands);
        Threads::userInput.clear();
        Threads::readyForInput = true;
    }
}
