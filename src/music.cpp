#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <filesystem>

std::filesystem::path getHome() {
#ifdef __unix__
    return std::getenv("HOME");
#elifdef _WIN32
    return std::getenv("USERPROFILE");
#endif
}

sf::Music Music::music{};
std::filesystem::path Music::musicDir{getHome() / "music"};
