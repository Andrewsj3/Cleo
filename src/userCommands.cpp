#include "userCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <exception>
#include <filesystem>
#include <flat_map>
#include <print>
#include <set>
#include <string>
#include <string_view>
#include <vector>
using CommandDefinition = std::flat_map<std::string, std::string>;
std::string join(const std::vector<std::string>&, std::string_view);
static const std::vector<std::string> commands{"exit", "help", "list", "loop",  "pause",
                                               "play", "stop", "time", "volume"};
static const CommandDefinition programHelp{
    {"play",
     R"(Usage: play <song>
Looks for a song in the music directory (default ~/music)
You can also type the first part of the song and Cleo will try to autocomplete it.)"},
    {"list", "Lists all songs in the music directory."},
    {"stop", "Stops the currently playing song."},
    {"pause", "Toggles whether the music should be paused or not."},
    {"exit", "Exits Cleo."},
    {"volume", R"(Usage: volume [newVolume]
With no arguments, shows the current volume.
Otherwise, sets the new volume provided it is between 0 and 100.)"},
    {"help", "Shows how commands work and how you can use Cleo."},
    {"commands", join(commands, "\n")},
    {"time", "Shows the current song's elapsed time and remaining time."},
    {"loop", "Toggles whether songs should loop when they reach the end."}};
static constexpr int VOLUME_TOO_LOW{-1};
static constexpr int VOLUME_TOO_HIGH{-2};

bool checkArgCount(const std::vector<std::string>& args, size_t expectedCount,
                   std::string_view errMsg) {
    if (args.size() == expectedCount)
        return true;
    std::println("{}", errMsg);
    return false;
}

void Cleo::play(Command& cmd) {
    if (!checkArgCount(cmd.arguments(), 1, "Expected exactly one song to play"))
        return;
    std::string song{cmd.arguments().at(0)};
    std::string match{};
    std::vector<std::string> matches{};
    std::filesystem::path songPath{Music::musicDir / song};
    // check for exact filename match including extension
    if (Music::load.openFromFile(songPath)) {
        (void)Music::music.openFromFile(songPath);
        Music::music.play();
        return;
    }
    // check for exact name match not including extension
    switch (autocomplete(Music::songs, song, match, matches)) {
    case Match::NoMatch:
        std::println("Song not found");
        return;
    case Match::ExactMatch:
        break;
    case Match::MultipleMatch:
        std::println("Multiple matches found, could be one of {}", join(matches, ", "));
        return;
    }
    bool foundSong{false};
    for (const auto& ext : Music::supportedExtensions) {
        if (Music::load.openFromFile(Music::musicDir / (match + ext))) {
            (void)Music::music.openFromFile(Music::musicDir / (match + ext));
            foundSong = true;
            break;
        }
    }
    if (!foundSong) {
        std::println("A match was found, but the file had an unsupported extension");
        return;
    }
    Music::music.play();
}

void Cleo::list(Command&) {
    std::stringstream sb{};
    std::set<std::string> directorySorted{};
    for (const auto& dirEntry : std::filesystem::directory_iterator(Music::musicDir)) {
        std::string ext{dirEntry.path().extension()};
        if (Music::supportedExtensions.contains(ext)) {
            directorySorted.insert(dirEntry.path().stem().string());
        }
    }
    for (const auto& dirEntry : directorySorted) {
        sb << dirEntry << ", ";
    }
    std::string dirList{sb.str()};
    dirList.erase(dirList.size() - 2);
    std::println("{}", dirList);
}

void Cleo::stop(Command&) { Music::music.stop(); }

void Cleo::exit(Command&) { Threads::running = false; }

void getVolume() {
    float curVolume{Music::music.getVolume()};
    std::println("Volume: {}%", curVolume);
}

void setVolume(const std::string& strVolume) {
    float newVolume{};
    try {
        newVolume = std::stof(strVolume);
        if (newVolume < 0) {
            throw VOLUME_TOO_LOW;
        } else if (newVolume > 100) {
            throw VOLUME_TOO_HIGH;
        } else {
            Music::music.setVolume(newVolume);
        }
    } catch (const std::exception&) {
        std::println("Value given was not a number");
        return;
    } catch (const int x) {
        if (x == VOLUME_TOO_LOW) {
            std::println("Volume cannot be below 0");
        } else if (x == VOLUME_TOO_HIGH) {
            std::println("Volume cannot be above 100");
        }
        return;
    }
}

void Cleo::volume(Command& cmd) {
    if (cmd.arguments().size() == 0) {
        getVolume();
    } else {
        setVolume(cmd.arguments().at(0));
    }
}

void Cleo::pause(Command&) {
    using Status = sf::Music::Status;
    Status curStatus{Music::music.getStatus()};
    switch (curStatus) {
    case Status::Playing:
        Music::music.pause();
        break;
    case Status::Paused:
        Music::music.play();
        break;
    case Status::Stopped:
        std::println("Cannot pause or unpause while music is stopped");
        break;
    }
}

std::string join(const std::vector<std::string>& vec, std::string_view delim) {
    if (vec.size() == 0) {
        return "";
    } else if (vec.size() == 1) {
        return vec.at(0);
    }
    std::string joined{};
    for (std::vector<std::string>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
        joined += *it;
        if (it != vec.end() - 1) {
            joined += delim;
        }
    }
    return joined;
}

void findHelp(const std::string& topic) {
    if (topic == "quit") {
        if (Threads::helpMode) {
            Threads::helpMode = false;
        } else {
            std::println("No help found for 'quit'");
        }
        return;
    }
    std::string match{};
    std::vector<std::string> matches{};
    if (programHelp.contains(topic)) {
        std::println("{}", programHelp.at(topic));
    } else {
        switch (autocomplete(programHelp.keys(), topic, match, matches)) {
        case Match::NoMatch:
            std::println("No help found for '{}'", topic);
            break;
        case Match::ExactMatch:
            std::println("{}", programHelp.at(match));
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}", join(matches, ", "));
            break;
        }
    }
}
void Cleo::help(Command& cmd) {
    if (cmd.arguments().size() == 0 && !Threads::helpMode) {
        std::println("Welcome to Cleo's interactive help utility.");
        std::println("Type `commands` to see the list of commands.");
        std::println("Type `quit` or CTRL-D to return to Cleo.");
        Threads::helpMode = true;
        return;
    }
    std::string topic{};
    if (Threads::helpMode) {
        std::vector<std::string> args{cmd.function()};
        args.append_range(cmd.arguments());
        topic = join(args, " ");
    } else {
        topic = join(cmd.arguments(), " ");
    }
    findHelp(topic);
}

void Cleo::time(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Stopped) {
        std::println("Nothing playing");
    } else {
        int timeElapsed{(int)Music::music.getPlayingOffset().asSeconds()};
        auto [elapsedMins, elapsedSecs]{std::div(timeElapsed, 60)};
        int remaining{(int)Music::music.getDuration().asSeconds() - timeElapsed};
        auto [remainingMins, remainingSecs]{std::div(remaining, 60)};
        std::println("{0:}:{1:02} elapsed, {2:}:{3:02} remaining", elapsedMins, elapsedSecs,
                     remainingMins, remainingSecs);
    }
}

void Cleo::loop(Command&) {
    Music::music.setLooping(!Music::music.isLooping());
    std::println("Looping: {}", Music::music.isLooping() ? "enabled" : "disabled");
}
