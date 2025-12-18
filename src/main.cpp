#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <print>
#define CLEO_VERSION "1.4.0"

int main() {
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    if (shouldRunWizard()) {
        runWizard();
    }
    updateScripts();
    updateSongs();
    updatePlaylists();
    readCache();
    runThreads();
    writeCache();
    return 0;
}
