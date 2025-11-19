#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
std::filesystem::path getHome() { return std::getenv("HOME"); }

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
    int repeats{};
    std::string curSong{};
    std::size_t playlistIdx{};
    bool inPlaylistMode{false};
} // namespace Music

void updateSongs() {
    std::string filename{};
    std::vector<std::string> newSongs{};
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
