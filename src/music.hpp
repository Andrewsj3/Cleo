#pragma once

#include "SFML/Audio/Music.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <unordered_set>

namespace Music {
    extern sf::Music music;
    extern std::filesystem::path musicDir;
    extern std::filesystem::path playlistDir;
    extern std::filesystem::path scriptDir;
    extern const std::unordered_set<std::string> supportedExtensions;
    extern std::vector<std::string> songs;
    extern std::vector<std::string> scripts;
    extern std::vector<std::string> playlists;
    extern std::vector<std::string> curPlaylist;
    extern std::vector<std::string> shuffledPlaylist;
    extern std::map<std::filesystem::path, int> songDurations;
    extern int repeats;
    extern std::string curSong;
    extern std::size_t playlistIdx;
    extern bool inPlaylistMode;
    extern bool isShuffled;
    extern bool isPlaylistLooping;
    extern bool isExecutingScript;
    extern std::string prompt;
} // namespace Music

void updateSongs();
void writeCache();
void updatePlaylists();
void updateScripts();
const std::vector<std::string>& getPlaylist();
