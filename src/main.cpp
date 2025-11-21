#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#define CLEO_VERSION "0.12.0"

int main() {
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    updateSongs();
    updatePlaylists();
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    runThreads();
    writeCache();
    return 0;
}
