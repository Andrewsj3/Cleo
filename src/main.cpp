#include "input.hpp"
#include "music.hpp"
#include "statMusic.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#include <thread>
#define CLEO_VERSION "0.10.0"

int main() {
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    updateSongs();
    updatePlaylists();
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    std::thread statThreadObj{monitorChanges};
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread};

    statThreadObj.detach();
    // Detach is needed for program to exit immediately at user's request
    inputThreadObj.join();
    backgroundThreadObj.join();
    return 0;
}
