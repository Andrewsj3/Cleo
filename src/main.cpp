#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#define CLEO_VERSION "1.3.0"

int main() {
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    updateScripts();
    updateSongs();
    updatePlaylists();
    readCache();
    runThreads();
    writeCache();
    return 0;
}
