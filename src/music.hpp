#pragma once

#include "SFML/Audio/Music.hpp"
#include <filesystem>

namespace Music {
    extern sf::Music music;
    extern std::filesystem::path musicDir;
} // namespace Music
