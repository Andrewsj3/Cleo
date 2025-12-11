#include "music.hpp"
#include "command.hpp"
#include "defaultCommands.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Time.hpp>
#include <fstream>
#include <iostream>
#include <print>

namespace fs = std::filesystem;
fs::path getHome() { return std::getenv("HOME"); }
static const fs::path cacheDir{getHome() / ".cache" / "cleo"};
static const fs::path cachePath{cacheDir / "cache"};
static constexpr int cacheSize{1000};

static std::map<fs::path, int> readCache() {
    if (!fs::exists(cachePath)) {
        fs::create_directories(cacheDir);
        std::ofstream{cachePath}.flush();
        return {};
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
        cache.insert({Music::musicDir / path, duration});
    }
    return cache;
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
    std::map<fs::path, int> songDurations{readCache()};
    int repeats{};
    std::string curSong{};
    std::size_t playlistIdx{};
    bool inPlaylistMode{false};
    bool isShuffled{false};
    bool isPlaylistLooping{false};
    bool isExecutingScript{false};
    std::string prompt{"> "};
} // namespace Music

void updateSongs() {
    while (!fs::exists(Music::musicDir)) {
        std::string musicDir{};
        std::print("Default music directory ({0}) does not exist, please specify one here or leave blank "
                   "to create {0}: ",
                   Music::musicDir.string());
        std::getline(std::cin, musicDir);
        if (musicDir.empty()) {
            fs::create_directories(Music::musicDir);
            std::println("{} created.", Music::musicDir.string());
            break;
        }
        Command cmd{"_", musicDir};
        // We don't need the function component, only the argument
        Cleo::setMusicDir(cmd);
    }
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
    while (!fs::exists(Music::playlistDir)) {
        std::string playlistDir{};
        std::print("Default playlist directory ({0}) does not exist, please specify one here or leave blank "
                   "to create {0}: ",
                   Music::playlistDir.string());
        std::getline(std::cin, playlistDir);
        if (playlistDir.empty()) {
            fs::create_directories(Music::playlistDir);
            std::println("{} created.", Music::playlistDir.string());
            break;
        }
        Command cmd{"_", playlistDir};
        Cleo::setPlaylistDir(cmd);
    }
    std::string playlist{};
    std::vector<std::string> newPlaylists{};
    for (const auto& dirEntry : fs::directory_iterator{Music::playlistDir}) {
        if (!dirEntry.is_regular_file()) {
            continue;
        }
        playlist = dirEntry.path().filename();
        newPlaylists.push_back(playlist);
    }
    Music::playlists = newPlaylists;
}

void updateScripts() {
    static const fs::path cleoStartup{Music::scriptDir / "startup"};
    if (!fs::exists(Music::scriptDir)) {
        fs::create_directories(Music::scriptDir);
        return;
    }
    if (!fs::exists(cleoStartup)) {
        std::ofstream startupScript{cleoStartup};
        startupScript << "# Any commands entered below will be executed when Cleo starts\n";
        startupScript.close();
    }
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
