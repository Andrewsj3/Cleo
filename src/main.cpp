#include "command.hpp"
#include "defaultCommands.hpp"
#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <cstring>
#include <print>
#define CLEO_VERSION "1.6.0"

void printUsage() {
    std::println("Usage: cleo [SCRIPTS]");
    std::println("A simple command-line music player for playing locally stored music and playlists.");
    std::println("A script can be either a full path or the name of a script inside ~/.config/cleo.");
    std::println("Type `help` inside cleo to learn more.");
    std::exit(1);
}

void handleArgs(int argc, char** argv) {
    if (argc == 1) {
        return;
    }
    std::vector<std::string> args{};
    for (int i{1}; i < argc; ++i) {
        if (!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help")) {
            printUsage();
        } else {
            args.push_back(argv[i]);
        }
    }
    Command cmd{"_", args};
    Cleo::run(cmd);
}

int main(int argc, char** argv) {
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    if (shouldRunWizard()) {
        runWizard();
    }
    updateScripts();
    updateSongs();
    updatePlaylists();
    readCache();
    handleArgs(argc, argv);
    runThreads();
    writeCache();
    return 0;
}
