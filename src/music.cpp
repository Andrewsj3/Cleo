#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <filesystem>
#include <string>
#include <vector>

std::filesystem::path getHome() { return std::getenv("HOME"); }

namespace Music {
    sf::Music music{};
    sf::Music load{};
    const std::filesystem::path musicDir{getHome() / "music"};
    const std::unordered_set<std::string> supportedExtensions{".mp3", ".ogg", ".flac", ".wav", ".aiff"};
    std::vector<std::string> songs{};
    int repeats{};
    std::string curSong{};
} // namespace Music

void updateSongs() {
    std::string filename{};
    std::vector<std::string> newSongs{};
    for (const auto& dirEntry : std::filesystem::directory_iterator(Music::musicDir)) {
        if (!dirEntry.is_regular_file() ||
            !Music::supportedExtensions.contains(dirEntry.path().extension())) {
            continue;
        }
        filename = dirEntry.path().stem().string();
        newSongs.push_back(filename);
    }
    Music::songs = newSongs;
}
