#include "playlistCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "defaultCommands.hpp"
#include "music.hpp"
#include <SFML/Audio/Music.hpp>
#include <fstream>
#include <iostream>
#include <print>
#include <random>
#include <regex>

using CommandDefinition = std::flat_map<std::string, std::string>;
using CommandMap = std::flat_map<std::string, std::function<void(Command&)>>;
using namespace Cleo;
namespace fs = std::filesystem;
static std::random_device rd{std::random_device{}};
static std::default_random_engine rng{std::default_random_engine{rd()}};
const std::vector<std::string> Playlist::commandList{
    "add", "clear", "find", "load", "loop", "next", "play", "previous", "save", "shuffle", "status",
};
const CommandMap Playlist::commands{
    {"load", Playlist::load}, {"play", Playlist::play},     {"add", Playlist::add},
    {"save", Playlist::save}, {"status", Playlist::status}, {"shuffle", Playlist::shuffle},
    {"find", Playlist::find}, {"next", Playlist::next},     {"previous", Playlist::previous},
    {"loop", Playlist::loop}, {"clear", Playlist::clear},
};

const CommandDefinition Playlist::commandHelp{
    {"load", R"(Usage: playlist load <filename>
Loads the songs in <filename> into the current playlist.
Playlists are stored in ~/music/playlists by default.)"},
    {"save", R"(Usage: playlist save <filename>
Saves the current playlist to the file chosen. Note that it automatically adds the csv
extension, so you don't need to specify one yourself.)"},
    {"play", R"(Starts playing the playlist and advances the song index by 1.
This means calling `playlist play` again skips to the next song, unless the current song
is the last song, in which case it will loop to the beginning.)"},
    {"add", R"(Usage: playlist add <song>
Adds the specified song to the end of the current playlist.)"},
    {"commands", join(Playlist::commandList, "\n")},
    {"status", R"(Shows the current song being played, as well as the previous and next
songs if applicable. Also displays current time elapsed and total length of playlist.)"},
    {"shuffle", R"(Toggles between the shuffled playlist and the normal playlist. Note everytime shuffle is
turned on, the order changes.)"},
    {"find", R"(Usage: playlist find [song]|[index]
Prints the song's position in the playlist, as well as the previous and next song if applicable.
It also shows the previous 5 songs and the next 5 songs with the current song in bold and underline.
If an index is given, it prints the song at the given index in the playlist. If nothing is given,
it defaults to the current song being played.)"},
    {"next", "Plays the next song in the playlist as long as the end of the playlist has not been reached."},
    {"previous", "Plays the previous song in the playlist unless the playlist is at the first song."},
    {"loop", "Toggles whether the playlist should loop after reaching the end."},
    {"clear", "Removes all songs from the playlist."},
};

static void playSong(const fs::path& songPath) {
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
static void parsePlaylist(const fs::path& path) {
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
    Music::shuffledPlaylist = Music::curPlaylist = playlist;
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
    std::vector<std::string> basePlaylistNames{};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Playlist not found.");
            break;
        case Match::ExactMatch:
            playlistPath = Music::playlistDir / match.exactMatch();
            parsePlaylist(playlistPath);
            break;
        case Match::MultipleMatch:
            basePlaylistNames.resize(match.matches.size());
            std::transform(match.matches.begin(), match.matches.end(), basePlaylistNames.begin(), stem);
            std::println("Multiple matches found, could be one of {}.", join(basePlaylistNames, ", "));
            break;
    }
}

void Playlist::play(Command&) {
    const std::vector<std::string>& playlist{getPlaylist()};
    if (playlist.empty()) {
        std::println("Playlist is empty.");
        return;
    }
    if (Music::playlistIdx == playlist.size()) {
        Music::playlistIdx = 0;
    }
    Music::inPlaylistMode = true;
    Music::repeats = 0;
    playSong(Music::musicDir / playlist.at(Music::playlistIdx));
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
    const std::vector<std::string>& playlist{getPlaylist()};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch:
            songPath = Music::musicDir / match.exactMatch();
            if (std::find(playlist.cbegin(), playlist.cend(), songPath.filename()) != playlist.cend()) {
                std::println("Song is already in playlist.");
                break;
            }
            Music::curPlaylist.push_back(songPath.filename());
            Music::shuffledPlaylist.push_back(songPath.filename());
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

static void printPreviousNextSong() {
    std::string prevSong{"N/A"};
    std::string nextSong{"N/A"};
    const std::vector<std::string>& playlist{getPlaylist()};
    if (Music::playlistIdx > 1) {
        prevSong = stem(playlist[Music::playlistIdx - 2]);
    }
    if (Music::playlistIdx < playlist.size()) {
        nextSong = stem(playlist[Music::playlistIdx]);
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
    const std::vector<std::string>& playlist{getPlaylist()};
    for (std::size_t i{0}; i < playlist.size(); ++i) {
        thisDuration = Music::songDurations.at(Music::musicDir / playlist[i]);
        totalTime += thisDuration;
        if (i < Music::playlistIdx - 1) {
            timeElapsed += thisDuration;
        }
    }
    timeElapsed += (int)Music::music.getPlayingOffset().asSeconds();
    printPreviousNextSong();
    std::println("Currently playing {} ({}/{})", Music::curSong, Music::playlistIdx, playlist.size());
    std::println("Total length of playlist: {}", numAsTimestamp(totalTime));
    std::println("Total time elapsed: {} ({:.1f}%)", numAsTimestamp(timeElapsed),
                 ((float)timeElapsed / (float)totalTime) * 100);
}

void Playlist::shuffle(Command&) {
    if (Music::curPlaylist.empty()) {
        std::println("Empty playlist cannot be shuffled.");
        return;
    }
    if (Music::curPlaylist.size() == 1) {
        std::println("Cannot shuffle playlist with only one song.");
        return;
    }
    Music::isShuffled = !Music::isShuffled;
    if (Music::isShuffled) {
        std::ranges::shuffle(Music::shuffledPlaylist, rng);
    }
    std::println("Shuffle: {}.", Music::isShuffled ? "on" : "off");
}

std::string numAsPosition(long num) {
    std::string numStr{std::to_string(num)};
    static const std::regex thSpecialCase{R"(^\d*1[123]$)"};
    // Account for 11th, 12th, 13th, etc.
    if (std::regex_match(numStr, thSpecialCase)) {
        return numStr + "th";
    } else if (numStr.ends_with("1")) {
        return numStr + "st";
    } else if (numStr.ends_with("2")) {
        return numStr + "nd";
    } else if (numStr.ends_with("3")) {
        return numStr + "rd";
    } else {
        return numStr + "th";
    }
}

static void filterAndPrintSongs(const std::vector<std::string>& playlist,
                                std::vector<std::string>::const_iterator iter, std::string_view song) {
    constexpr std::size_t maxSongs{11}; // 5 preceding the song, 5 after it, plus the song itself
    std::vector<std::string> songs(maxSongs);
    long distFromStart{std::distance(playlist.begin(), iter)};
    long distFromEnd{std::distance(iter, playlist.end()) - 1};
    // For some reason we need to subtract one or it will try copying elements beyond the end
    long left{std::min(5L, distFromStart)};
    long right{std::min(5L, distFromEnd)};
    // Bounds checking to ensure we copy elements safely
    std::copy_n(iter - left, left + right + 1, songs.begin());
    auto target = std::find(songs.begin(), songs.end(), song);
    std::transform(songs.cbegin(), songs.cend(), songs.begin(), stem);
    *target = std::format("\x1b[4m\x1b[1m{}\x1b[0m", *target); // bold and underline
    std::print("{} is {} in the playlist, ", stem(song), numAsPosition(distFromStart + 1));
    if (distFromStart == 0) {
        std::string before{stem(*(target + 1))};
        std::println("before {}", before);
    } else if (distFromEnd == 0) {
        std::string after{stem(*(target - 1))};
        std::println("after {}", after);
    } else {
        std::string before{stem(*(target + 1))};
        std::string after{stem(*(target - 1))};
        // Note we can't define these before the condition otherwise we could get a segfault if we are at the beginning or end of a playlist
        std::println("before {}, and after {}", before, after);
    }
    std::println("\n...{}...", join(songs, ", "));
}

static bool isDigit(std::string_view num) { return num.find_first_not_of("0123456789") == std::string::npos; }

void Playlist::find(Command& cmd) {
    const std::vector<std::string>& playlist{getPlaylist()};
    if (playlist.empty() || !Music::inPlaylistMode) {
        std::println("Not currently playing a playlist.");
        return;
    }
    std::string song{};
    if (cmd.argCount() == 0) {
        song = playlist[Music::playlistIdx - 1];
    } else {
        song = cmd.nextArg();
    }
    if (isDigit(song)) {
        size_t index{std::stoull(song)};
        if (0 < index && index < playlist.size() + 1) {
            song = playlist.at(index - 1);
        } else {
            std::println("Please enter a valid position between 1-{}.", playlist.size());
            return;
        }
    } else {
        AutoMatch match{playlist, song};
        switch (match.matchType) {
            case Match::NoMatch:
                std::println("Song not found in playlist.");
                return;
            case Match::ExactMatch:
                song = match.exactMatch();
                break;
            case Match::MultipleMatch:
                std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
                return;
        }
    }
    if (playlist.size() == 1) {
        std::println("{} is 1st in the playlist.", song);
        return;
    }
    auto posIter{std::find(playlist.begin(), playlist.end(), song)};
    filterAndPrintSongs(playlist, posIter, song);
}

void Playlist::next(Command& _) {
    if (!Music::inPlaylistMode) {
        std::println("Not currently playing a playlist.");
        return;
    }
    if (Music::playlistIdx == Music::curPlaylist.size() && !Music::isPlaylistLooping) {
        // Allow user to go forward if playlist is set to loop, which will send them back to the start
        std::println("End of playlist reached.");
        return;
    }
    play(_);
}

void Playlist::previous(Command& _) {
    if (!Music::inPlaylistMode) {
        std::println("Not currently playing a playlist.");
        return;
    }
    if (Music::playlistIdx > 1) {
        // Don't let user go back even if playlist is looping because it wouldn't make sense
        Music::playlistIdx -= 2;
        play(_);
    } else {
        std::println("Can't go back any further.");
    }
}

void Playlist::loop(Command&) {
    Music::isPlaylistLooping = !Music::isPlaylistLooping;
    std::println("Playlist loop: {}.", Music::isPlaylistLooping ? "enabled" : "disabled");
}

void Playlist::clear(Command&) {
    Music::curPlaylist.clear();
    Music::shuffledPlaylist.clear();
    Music::inPlaylistMode = false;
    Music::playlistIdx = 0;
    std::println("Playlist cleared.");
}
