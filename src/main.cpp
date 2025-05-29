#include "input.hpp"
#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#include <thread>
#define CLEO_VERSION "0.4.0"

int main() {
    sf::err().rdbuf(nullptr);
    updateSongs();
    std::println("Cleo " CLEO_VERSION ", powered by SFML");
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread};

    inputThreadObj.join();
    backgroundThreadObj.join();
    return 0;
}
