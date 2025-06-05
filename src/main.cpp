#include "input.hpp"
#include "music.hpp"
#include "statMusic.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#include <thread>
#define CLEO_VERSION "0.7.0"

int main() {
    sf::err().rdbuf(nullptr);
    updateSongs();
    std::println("Cleo " CLEO_VERSION ", powered by SFML");
    std::thread statThreadObj{monitorChanges};
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread};

    statThreadObj.detach();
    inputThreadObj.join();
    backgroundThreadObj.join();
    return 0;
}
