#include "command.hpp"
#include "defaultCommands.hpp"
#include "music.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System.hpp>
#include <getopt.h>
#include <print>
#include <readline/tilde.h>
#define CLEO_VERSION "1.9.0"

namespace fs = std::filesystem;

static int wizard_flag{0};
static const struct option long_options[] = {
    {"prompt", required_argument, nullptr, 'p'},
    {"music-dir", required_argument, nullptr, 'm'},
    {"playlist-dir", required_argument, nullptr, 'P'},
    {"help", no_argument, &wizard_flag, 'h'},
    {"wizard", no_argument, nullptr, 'w'},
    {"version", no_argument, nullptr, 'v'},
    {0, 0, 0, 0},
};

bool setDirectory(const char* path, fs::path& directory) {
    fs::path newDir{tilde_expand(path)};
    if (!fs::exists(newDir) || !fs::is_directory(newDir)) {
        return false;
    } else {
        directory = newDir;
        return true;
    }
}

void printUsage() {
    std::println("Usage: cleo [OPTIONS] [SCRIPTS]");
    std::println("A simple command-line music player for playing locally stored music and playlists.");
    std::println("A script can be either a full path or the name of a script inside ~/.config/cleo.");
    std::println("Type `help` inside cleo to learn more.");
    std::println();
    std::println("  -m, --music-dir=DIR");
    std::println("\tSet the music directory to DIR");
    std::println("  -p, --prompt=STR");
    std::println("\tSet the prompt to STR");
    std::println("  -P, --playlist-dir=DIR");
    std::println("\tSet the playlist directory to DIR");
    std::println("  -w, --wizard");
    std::println("\tRun the setup wizard, overriding any previous configuration");
    std::println("  -h, --help");
    std::println("\tShow this help and exit");
    std::println("  -v, --version");
    std::println("\tShow version information and exit");
    std::exit(0);
}

void handleArgs(int argc, char** const argv) {
    int val;
    while ((val = getopt_long(argc, argv, ":hvwm:p:P:", long_options, nullptr)) != -1) {
        switch (val) {
            case 'h':
                printUsage();
                break;
            case 'm':
                if (!isValidDirectory(optarg)) {
                    std::println("Error: Given music directory {} is invalid.", optarg);
                    exit(1);
                }
                fs::create_directories(optarg);
                Music::musicDir = optarg;
                break;
            case 'p':
                Music::prompt = optarg;
                break;
            case 'P':
                if (!isValidDirectory(optarg)) {
                    std::println("Error: Given playlist directory {} is invalid.", optarg);
                    exit(1);
                }
                fs::create_directories(optarg);
                Music::playlistDir = optarg;
                break;
            case 'w':
                runWizard();
                break;
            case 'v':
                std::println("Cleo version: {}\nSFML version: {}.{}.{}", CLEO_VERSION, SFML_VERSION_MAJOR,
                             SFML_VERSION_MINOR, SFML_VERSION_PATCH);
                exit(0);
            case '?':
                std::println("Unknown option `{}`", argv[optind - 1]);
                std::println("Try `cleo --help` for a list of available options.");
                exit(1);
            case ':':
                std::println("`{}` requires an argument", argv[optind - 1]);
                std::println("Try `cleo --help` for more information.");
                break;
        }
    }
    std::println("Cleo " CLEO_VERSION ", powered by SFML.");
    if (optind == argc) {
        return; // no scripts to run
    }
    std::vector<std::string> args{};
    for (int i{optind}; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    Command cmd{"_", args};
    Cleo::run(cmd);
}

int main(int argc, char** const argv) {
    sf::err().rdbuf(nullptr); // Silence SFML errors, we provide our own.
    updateScripts();
    handleArgs(argc, argv);
    if (shouldRunWizard(wizard_flag)) {
        runWizard();
    }
    updateSongs();
    updatePlaylists();
    readCache();
    runThreads();
    writeCache();
    return 0;
}
