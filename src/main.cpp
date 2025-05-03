#include "command.hpp"
#include "input.hpp"
#include "music.hpp"
#include "userCommands.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <flat_map>
#include <iostream>
#include <print>
#include <thread>
#define CLEO_VERSION "0.3.0"

int main() {
    using commandMap = std::flat_map<std::string, std::function<void(Command&)>>;
    sf::err().rdbuf(nullptr);
    commandMap programCommands{
        {"exit", Cleo::exit}, {"list", Cleo::list}, {"pause", Cleo::pause},
        {"play", Cleo::play}, {"stop", Cleo::stop}, {"volume", Cleo::volume},
    };
    updateSongs();
    std::println("Cleo " CLEO_VERSION ", powered by SFML");
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread, programCommands};

    inputThreadObj.join();
    backgroundThreadObj.join();
    return 0;
}
