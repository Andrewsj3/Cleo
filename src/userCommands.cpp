#include "userCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <print>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
constexpr int VOLUME_TOO_LOW{-1};
constexpr int VOLUME_TOO_HIGH{-2};

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
    std::filesystem::path songPath{Music::musicDir / song};
    // check for exact filename match including extension
    if (Music::load.openFromFile(songPath)) {
        (void)Music::music.openFromFile(songPath);
        Music::music.play();
        return;
    }
    // check for exact name match not including extension
    switch (autocomplete(Music::songs, song, match)) {
    case Match::NoMatch:
        std::println("Song not found");
        return;
    case Match::ExactMatch:
        break;
    case Match::MultipleMatch:
        std::println("Multiple matches found, try refining your search");
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

void Cleo::exit(Command&) {
    Threads::running = false;
}

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
