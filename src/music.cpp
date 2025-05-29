#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <filesystem>
#include <string>
#include <vector>

std::filesystem::path getHome() { return std::getenv("HOME"); }

sf::Music Music::music{};
sf::Music Music::load{};
const std::filesystem::path Music::musicDir{getHome() / "music"};
const std::unordered_set<std::string> Music::supportedExtensions{".mp3", ".ogg", ".flac", ".wav"};
std::vector<std::string> Music::songs{};

void updateSongs() {
    std::string filename{};
    std::vector<std::string> newSongs{};
    for (const auto& dirEntry: std::filesystem::directory_iterator(Music::musicDir)) {
		if (!dirEntry.is_regular_file()) {
			continue;
		}
        filename = dirEntry.path().stem().string();
        newSongs.push_back(filename);
    }
    Music::songs = newSongs;
}
