#include "command.hpp"
#include "userCommands.hpp"
#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <cstdlib>
#include <exception>
#include <print>
#include <string>
#include <string_view>
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
    std::string_view song{cmd.arguments().at(0)};
    std::filesystem::path songPath{Music::musicDir / song};
    if (!Music::music.openFromFile(songPath)) {
        std::println("Cound not play file '{}'", song);
        return;
    }
    Music::music.play();
}

void Cleo::stop(Command&) { Music::music.stop(); }

void Cleo::exit(Command&) {}

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
