#pragma once

#include "SFML/Audio/Music.hpp"
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Music {
    extern sf::Music music;
    extern const std::filesystem::path musicDir;
    extern const std::filesystem::path playlistDir;
    extern const std::unordered_set<std::string> supportedExtensions;
    extern std::vector<std::string> songs;
    extern std::vector<std::string> playlists;
    extern std::vector<std::string> curPlaylist;
    extern int repeats;
    extern std::string curSong;
    extern std::size_t playlistIdx;
    extern bool inPlaylistMode;
} // namespace Music

void updateSongs();
void updatePlaylists();
