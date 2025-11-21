#include "playlistCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "defaultCommands.hpp"
#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <algorithm>
#include <filesystem>
#include <flat_map>
#include <fstream>
#include <iostream>
#include <ostream>
#include <print>
#include <string>
#include <vector>

using CommandDefinition = std::flat_map<std::string, std::string>;
using CommandMap = std::flat_map<std::string, std::function<void(Command&)>>;
using namespace Cleo;
namespace fs = std::filesystem;
const std::vector<std::string> Playlist::commandList{
    "add", "load", "play", "save", "status",
};
const CommandMap Playlist::commands{
    {"load", Playlist::load}, {"play", Playlist::play},     {"add", Playlist::add},
    {"save", Playlist::save}, {"status", Playlist::status},
};
const CommandDefinition Playlist::commandHelp{
    {"load", R"(Usage: playlist load <filename>
Loads the songs in <filename> into the current playlist.
Playlists are stored in ~/music/playlists by default.)"},
    {"save", R"(Usage: playlist save <filename>
Saves the current playlist to the file chosen. Note that it automatically adds the csv extension,
so you don't need to specify one yourself.)"},
    {"play", R"(Starts playing the playlist and advances the song index by 1.
This means calling `playlist play` again skips to the next song, unless the current song is the last song,
in which case it will loop to the beginning.)"},
    {"add", R"(Usage: playlist add <song>
Adds the specified song to the end of the current playlist.)"},
    {"commands", join(Playlist::commandList, "\n")},
    {"status", R"(Shows the current song being played, as well as the previous and next songs if applicable.
Also displays current time elapsed and total length of playlist.)"},
};

void playSong(const fs::path& songPath) {
    if (fs::exists(songPath)) {
        if (Music::music.openFromFile(songPath)) {
            Music::curSong = songPath.filename().stem();
            Music::music.play();
        } else {
            std::println("File is in an unsupported format.");
            std::print("> ");
        }
    } else {
        std::println("Song not found.");
        std::print("> ");
    }
    // We need to print the prompt here otherwise the output will get messed up
    std::flush(std::cout);
}

// Reads csv file into playlist. Assumes csv was created by cleo and that it only has 1 line.
void parsePlaylist(const fs::path& path) {
    std::ifstream file{path};
    std::string curItem{};
    std::vector<std::string> playlist{};
    sf::Music load{};
    while (std::getline(file, curItem, ',')) {
        if (file.eof()) {
            // Account for dos and unix line endings. This is mainly for compatibility with smp.
            if (curItem.ends_with("\r\n")) {
                curItem.erase(curItem.length() - 2, 2);
            } else if (curItem.ends_with("\n")) {
                curItem.erase(curItem.length() - 1, 1);
            } else {
                std::println("Unknown line ending encountered when parsing playlist.");
                return;
            }
        }
        if (!Music::songDurations.contains(Music::musicDir / curItem)) {
            if (load.openFromFile(Music::musicDir / curItem)) {
                Music::songDurations.insert({Music::musicDir / curItem, load.getDuration().asSeconds()});
            }
        }
        if (!fs::exists(Music::musicDir / curItem)) {
            std::println("Song not found: {}", (Music::musicDir / curItem).string());
        } else {
            playlist.push_back(curItem);
        }
    }
    Music::curPlaylist = playlist;
    Music::playlistIdx = 0;
}

void Playlist::load(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected the name of a playlist.");
        return;
    }
    std::string playlist{cmd.nextArg()};
    if (fs::exists(Music::playlistDir / playlist)) {
        parsePlaylist(Music::playlistDir / playlist);
        return;
    }
    AutoMatch match{Music::playlists, playlist};
    fs::path playlistPath{};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Playlist not found.");
            break;
        case Match::ExactMatch:
            playlistPath = Music::playlistDir / match.exactMatch();
            parsePlaylist(playlistPath);
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
            break;
    }
}

void Playlist::play(Command&) {
    if (Music::curPlaylist.empty()) {
        std::println("Playlist is empty.");
        return;
    }
    if (Music::playlistIdx == Music::curPlaylist.size()) {
        Music::playlistIdx = 0;
    }
    Music::inPlaylistMode = true;
    Music::repeats = 0;
    playSong(Music::musicDir / Music::curPlaylist.at(Music::playlistIdx));
    // We use this instead of Cleo::play since we don't have an instance of Command
    ++Music::playlistIdx;
}

void Playlist::add(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected a song to add to the current playlist.");
        return;
    }
    std::string song{cmd.nextArg()};
    AutoMatch match{Music::songs, song};
    fs::path songPath{};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch:
            songPath = Music::musicDir / match.exactMatch();
            if (std::find(Music::curPlaylist.cbegin(), Music::curPlaylist.cend(), songPath.filename()) !=
                Music::curPlaylist.cend()) {
                std::println("Song is already in playlist.");
                break;
            }
            Music::curPlaylist.push_back(songPath.filename());
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
            break;
    }
}

void Playlist::save(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected a name for the playlist.");
        return;
    }
    std::string playlistName{cmd.nextArg()};
    fs::path destination{Music::playlistDir / (playlistName + ".csv")};
    // Don't make the user enter an extension themselves, although technically they still can
    std::ofstream output;
    if (fs::exists(destination)) {
        std::string choice{};
        std::print("Playlist already exists, do you want to overwrite it? [y/N] ");
        std::getline(std::cin, choice);
        if (choice == "y" || choice == "Y") {
            output.open(destination);
            output << join(Music::curPlaylist, ",") << '\n';
            // Make sure to save with LF line ending since this is what the load function
            // expects
            std::println("Playlist saved.");
        }
    } else {
        output.open(destination);
        output << join(Music::curPlaylist, ",") << '\n';
        std::println("Playlist saved.");
    }
    output.close();
}

void printPreviousNextSong() {
    std::string prevSong{"N/A"};
    std::string nextSong{"N/A"};
    if (Music::playlistIdx > 1) {
        prevSong = fs::path{Music::curPlaylist[Music::playlistIdx - 2]}.stem();
    }
    if (Music::playlistIdx < Music::curPlaylist.size()) {
        nextSong = fs::path{Music::curPlaylist[Music::playlistIdx]}.stem();
    }
    std::println("Previous song: {}, next song: {}", prevSong, nextSong);
}

void Playlist::status(Command&) {
    if (!Music::inPlaylistMode) {
        std::println("Not playing a playlist.");
        return;
    }
    int totalTime{0};
    int timeElapsed{0};
    int thisDuration{};
    for (std::size_t i{0}; i < Music::curPlaylist.size(); ++i) {
        thisDuration = Music::songDurations.at(Music::musicDir / Music::curPlaylist[i]);
        totalTime += thisDuration;
        if (i < Music::playlistIdx - 1) {
            timeElapsed += thisDuration;
        }
    }
    timeElapsed += (int)Music::music.getPlayingOffset().asSeconds();
    printPreviousNextSong();
    std::println("Currently playing {} ({}/{})", Music::curSong, Music::playlistIdx,
                 Music::curPlaylist.size());
    std::println("Total length of playlist: {}", numAsTimestamp(totalTime));
    std::println("Total time elapsed: {} ({:.1f}%)", numAsTimestamp(timeElapsed),
                 ((float)timeElapsed / (float)totalTime) * 100);
}
