#include "music.hpp"
#include "command.hpp"
#include "defaultCommands.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Time.hpp>
#include <fstream>
#include <iostream>
#include <print>
#include <readline/readline.h>
#include <wordexp.h>

namespace fs = std::filesystem;
fs::path getHome() { return std::getenv("HOME"); }
static const fs::path cacheDir{getHome() / ".cache" / "cleo"};
static const fs::path cachePath{cacheDir / "cache"};
static const fs::path firstTimeCheck{cacheDir / "no-wizard"};
static constexpr int cacheSize{1000};

static fs::path selectDirectory(std::string_view prompt) {
    bool succeeded{false};
    fs::path selectedDir{};
    std::println("{}", prompt);
    do {
        const char* input = readline("> ");
        if (input == NULL) {
            free((void*)input);
            exit(1);
        } else if (strlen(input) == 0) {
            std::println("Directory cannot be empty.");
            free((void*)input);
            continue;
        }

        wordexp_t p;
        int status = wordexp(input, &p, WRDE_NOCMD);
        free((void*)input);
        if (status != 0 || p.we_wordc == 0) {
            std::println("Invalid directory.");
            continue;
        }
        selectedDir = *p.we_wordv;
        wordfree(&p);
        if (!fs::exists(selectedDir)) {
            try {
                fs::create_directories(selectedDir);
                std::println("Directory created.");
            } catch (std::filesystem::filesystem_error) {
                std::println("Could not create specified directory, please try another directory.");
                continue;
            }
        }
        bool canAccess{false};
        std::ofstream test{selectedDir / "test.canwrite.whyareyoureadingthis"};
        // Test if we have write access by creating temporary file
        if (test.good()) {
            canAccess = true;
            test.close();
            fs::remove(selectedDir / "test.canwrite.whyareyoureadingthis");
        }
        if (!canAccess) {
            std::println("You do not have read and write access to this directory, please try another.");
            continue;
        }
        succeeded = true;
    } while (!succeeded);
    return selectedDir;
}

bool shouldRunWizard() { return !fs::exists(firstTimeCheck); }

void runWizard() {
    std::ofstream path{};
    std::ofstream configPath{Music::scriptDir / "startup"};
    std::println("Welcome to Cleo. Would you like to go through the inital setup? If not, defaults will be "
                 "used\n(see `help defaults` for more).");
    std::print("[Y/n] ");
    std::string doSetup{};
    std::getline(std::cin, doSetup);
    if (std::cin.eof()) {
        exit(1);
    }
    if (doSetup == "n" || doSetup == "N") {
        std::println("Using defaults.");
        fs::create_directories(Music::musicDir);
        fs::create_directories(Music::playlistDir);
        path.open(firstTimeCheck);
        path.flush();
        path.close();
        return;
    }
    Music::musicDir =
        selectDirectory("1/3: Select a music directory. The directory will be created if it does not exist.");
    configPath << "set-music " << Music::musicDir << '\n';
    Music::playlistDir = selectDirectory(
        "2/3: Select a playlist directory. The directory will be created if it does not exist.");
    configPath << "set-playlist " << Music::playlistDir << '\n';
    std::println("3/3: Select a prompt. Leave blank for the default (> )");
    std::getline(std::cin, Music::prompt);
    if (Music::prompt.empty()) {
        Music::prompt = "> ";
    }
    configPath << "set-prompt \"" << Music::prompt << "\"\n";
    std::println("Finished.");
    configPath.close();
    path.open(firstTimeCheck);
    path.flush();
    path.close();
}

void readCache() {
    if (!fs::exists(cachePath)) {
        fs::create_directories(cacheDir);
        std::ofstream{cachePath}.flush();
    }
    std::map<fs::path, int> cache{};
    std::ifstream inp{cachePath};
    std::string line{};
    std::filesystem::path path{};
    int duration{};
    while (std::getline(inp, line)) {
        std::size_t pos{line.find(':')};
        path = line.substr(0, pos);
        duration = std::stoi(line.substr(pos + 1));
        Music::songDurations.insert({path, duration});
        // It might seem like we should insert the music directory here, but if the user changes it, the whole
        // cache would get invalidated
    }
}

void writeCache() {
    std::ofstream cache{cachePath};
    int linesWritten{0};
    for (const auto& [path, duration] : Music::songDurations) {
        cache << path.filename().string() << ":" << duration << "\n";
        ++linesWritten;
        if (linesWritten == cacheSize) {
            break;
        }
    }
    cache.close();
}

namespace Music {
    sf::Music music{};
    fs::path musicDir{getHome() / "Music"};
    fs::path playlistDir{musicDir / "playlists"};
    fs::path scriptDir{getHome() / ".config" / "cleo"};
    const std::unordered_set<std::string> supportedExtensions{
        ".mp3", ".ogg", ".flac", ".wav", ".aiff",
    };
    // Taken from the list of formats that SFML supports. Some of the more obscure ones were
    // left out
    std::vector<std::string> songs{};
    std::vector<std::string> scripts{};
    std::vector<std::string> playlists{};
    std::vector<std::string> curPlaylist{};
    std::vector<std::string> shuffledPlaylist{};
    std::map<fs::path, int> songDurations{};
    int repeats{};
    std::string curSong{};
    std::string playlistCurName{};
    std::size_t playlistIdx{};
    bool inPlaylistMode{false};
    bool isShuffled{false};
    bool isPlaylistLooping{false};
    bool isExecutingScript{false};
    std::string prompt{"> "};
} // namespace Music

void updateSongs() {
    std::string filename{};
    std::vector<std::string> newSongs{};
    sf::Music load{};
    for (const auto& dirEntry : fs::directory_iterator{Music::musicDir}) {
        if (!dirEntry.is_regular_file() ||
            !Music::supportedExtensions.contains(dirEntry.path().extension())) {
            continue;
        }
        filename = dirEntry.path().filename();
        newSongs.push_back(filename);
    }
    Music::songs = newSongs;
    std::sort(Music::songs.begin(), Music::songs.end());
}

void updatePlaylists() {
    std::string playlist{};
    std::vector<std::string> newPlaylists{};
    for (const auto& dirEntry : fs::directory_iterator{Music::playlistDir}) {
        if (!dirEntry.is_regular_file() || !(dirEntry.path().extension() == ".csv")) {
            continue;
        }
        playlist = dirEntry.path().filename();
        newPlaylists.push_back(playlist);
    }
    Music::playlists = newPlaylists;
}

void updateScripts() {
    std::string script{};
    std::vector<std::string> scripts{};
    for (const auto& dirEntry : fs::directory_iterator{Music::scriptDir}) {
        if (!dirEntry.is_regular_file()) {
            continue;
        }
        script = dirEntry.path().filename();
        scripts.push_back(script);
    }
    Music::scripts = scripts;
    Command cmd{"_", "startup"};
    Cleo::run(cmd);
}

const std::vector<std::string>& getPlaylist() {
    return Music::isShuffled ? Music::shuffledPlaylist : Music::curPlaylist;
}
