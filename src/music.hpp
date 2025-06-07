#pragma once

#include "SFML/Audio/Music.hpp"
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Music {
    extern sf::Music music;
    extern sf::Music load;
    extern const std::filesystem::path musicDir;
    extern const std::unordered_set<std::string> supportedExtensions;
    extern std::vector<std::string> songs;
    extern int repeats;
    extern std::string curSong;
} // namespace Music

void updateSongs();
