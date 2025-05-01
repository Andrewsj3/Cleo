#include "command.hpp"
#include "input.hpp"
#include "userCommands.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <cstring>
#include <iostream>
#include <map>
#include <print>
#include <string>
#include <thread>
#define CLEO_VERSION "0.1.0"

int main() {
    using commandMap = std::map<std::string, std::function<void(Command&)>>;
    sf::err().rdbuf(nullptr);
    commandMap programCommands{
        {"exit", Cleo::exit}, {"pause", Cleo::pause},   {"play", Cleo::play},
        {"stop", Cleo::stop}, {"volume", Cleo::volume},
    };
    std::println("Cleo " CLEO_VERSION ", powered by SFML");
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread, programCommands};

    inputThreadObj.join();
    backgroundThreadObj.join();
    return 0;
}
