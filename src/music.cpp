#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Time.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

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
    const fs::path musicDir{getHome() / "music"};
    const fs::path playlistDir{musicDir / "playlists"};
    const std::unordered_set<std::string> supportedExtensions{
        ".mp3", ".ogg", ".flac", ".wav", ".aiff",
    };
    // Taken from the list of formats that SFML supports. Some of the more obscure ones were
    // left out
    std::vector<std::string> songs{};
    std::vector<std::string> playlists{};
    std::vector<std::string> curPlaylist{};
    std::vector<std::string> shuffledPlaylist{};
    std::map<fs::path, int> songDurations{readCache()};
    int repeats{};
    std::string curSong{};
    std::size_t playlistIdx{};
    bool inPlaylistMode{false};
    bool isShuffled{false};
} // namespace Music

void updateSongs() {
    std::string filename{};
    std::vector<std::string> newSongs{};
    sf::Music load{};
    for (const auto& dirEntry : fs::directory_iterator(Music::musicDir)) {
        if (!dirEntry.is_regular_file() ||
            !Music::supportedExtensions.contains(dirEntry.path().extension())) {
            continue;
        }
        filename = dirEntry.path().filename();
        newSongs.push_back(filename);
    }
    Music::songs = newSongs;
}

void updatePlaylists() {
    std::string playlist{};
    std::vector<std::string> newPlaylists{};
    for (const auto& dirEntry : fs::directory_iterator(Music::playlistDir)) {
        if (!dirEntry.is_regular_file()) {
            continue;
        }
        playlist = dirEntry.path().filename();
        newPlaylists.push_back(playlist);
    }
    Music::playlists = newPlaylists;
}

const std::vector<std::string>& getPlaylist() {
    return Music::isShuffled ? Music::shuffledPlaylist : Music::curPlaylist;
}
