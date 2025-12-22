#include "defaultCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "input.hpp"
#include "music.hpp"
#include "playlistCommands.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Time.hpp>
#include <cmath>
#include <fstream>
#include <print>
#include <regex>
#include <wordexp.h>
using CommandDefinition = std::flat_map<std::string, std::string>;
using CommandMap = std::flat_map<std::string, std::function<void(Command&)>>;
// Note this definition means each command has the same signature, even though some commands
// don't need arguments

namespace fs = std::filesystem;
const CommandMap Cleo::commands{
    {"exit", Cleo::exit},
    {"list", Cleo::list},
    {"pause", Cleo::pause},
    {"play", Cleo::play},
    {"stop", Cleo::stop},
    {"volume", Cleo::volume},
    {"help", Cleo::help},
    {"time", Cleo::time},
    {"loop", Cleo::loop},
    {"repeat", Cleo::repeat},
    {"rename", Cleo::rename},
    {"delete", Cleo::del},
    {"playlist", Cleo::playlist},
    {"queue", Cleo::playlist},
    {"seek", Cleo::seek},
    {"forward", Cleo::forward},
    {"rewind", Cleo::rewind},
    {"find", Cleo::find},
    {"set-music", Cleo::setMusicDir},
    {"set-playlist", Cleo::setPlaylistDir},
    {"set-prompt", Cleo::setPrompt},
    {"run", Cleo::run},
};
const std::vector<std::string> Cleo::commandList{
    "delete", "exit",      "find",         "forward",    "help",   "list",   "loop",
    "pause",  "play",      "playlist",     "rename",     "repeat", "rewind", "run",
    "seek",   "set-music", "set-playlist", "set-prompt", "stop",   "time",   "volume",
};
const CommandDefinition Cleo::commandHelp{
    {"play",
     R"(Usage: play <song>
Looks for a song in the music directory (default ~/music) and tries to play it.
You can also type the first part of the song and Cleo will try to autocomplete it.)"},
    {"list", "Lists all songs in the music directory."},
    {"stop", "Stops the currently playing song."},
    {"pause", "Toggles whether the music should be paused or not."},
    {"exit", "Exits Cleo."},
    {"volume", R"(Usage: volume [newVolume]
With no arguments, shows the current volume. Otherwise, sets the new volume provided
it is between 0 and 100.)"},
    {"help", "Shows how commands work and how you can use Cleo."},
    {"commands", join(Cleo::commandList, "\n")},
    {"time", "Shows the current song's elapsed time and remaining time."},
    {"loop", "Toggles whether songs should loop when they reach the end."},
    {"repeat", R"(Usage: repeat [numRepeats]
By default, repeats the song once.
Otherwise, repeats the song the given number of times provided it is at least 0.)"},
    {"rename", R"(Usage: rename <oldNames> <newNames>
Renames a song in the music directory (autocomplete is supported). This also applies to any
playlists with this song. Note the song must be in a supported format (see `help formats`)
or it will fail. The new song will have the same extension as the original, so don't
add an extension yourself. If you want to rename multiple songs, it works like this:
rename old1 new1 old2 new2 ...
THIS COMMAND DOES NOT CHECK IF A SONG WILL BE OVERWRITTEN.)"},
    {"formats", R"(Supported formats:
mp3
ogg
flac
wav
aiff)"},
    {"delete", R"(Usage: delete <songs>
Deletes each song from the music directory. Like `rename`, the song must be in a supported
format or it will not be deleted. This will also remove the song from all playlists.)"},
    {"autocomplete", R"(When typing a song, file, or command, you can type the first few
characters as long as it doesn't match anything else, e.g. `l` doesn't work because it
matches both `list` and `loop`. `li` works because it only matches list.)"},
    {"playlist", R"(Usage: playlist [subcommand] [argument]
This allows you to interact with the playlist in various ways.
If no subcommand is specified, it will show all songs in the playlist.
Do `playlist commands` to see all subcommands or `playlist <subcommand>` to see more
specific help. Note: you can also use the alias `queue` to make autocompletion easier.)"},
    {"seek", R"(Usage: seek <duration/timestamp>
Seeks to the specified duration or timestamp, only works if there is currently a song playing.
Accepts either a positive whole amount in seconds or a timestamp in one of these formats:
hh:mm:ss or mm:ss
Seeking past the end of the song goes straight to the end and stops playback.)"},
    {"forward", R"(Usage: forward <duration/timestamp>
Like seek, but takes current time elapsed into account and adds the given duration.)"},
    {"rewind", R"(Usage: rewind <duration/timestamp>
Like seek, but takes current time elapsed into account and subtracts the given duration.)"},
    {"find", R"(Usage: find <searches>
For each search term given, lists all songs that start with the specified search term.)"},
    {"set-music", R"(Usage: set-music [directory]
Instructs Cleo to search in this directory for songs, provided the directory exists.
To make this change permanent, put this command into ~/.config/cleo/startup
(see `run` for more))"},
    {"set-playlist", R"(Usage: set-playlist [directory]
Like `set-music`, but controls where to look for playlists.)"},
    {"run", R"(Usage: run <scripts>
Executes commands from the given scripts. Commands are in the same form as regular
Cleo commands. Lines beginning with a `#` are treated as comments and ignored.
Scripts are located in ~/.config/cleo
In particular, ~/.config/cleo/startup is automatically executed when Cleo starts,
so any changes you want to make permanent should go in there. However, scripts
cannot run other scripts.)"},
    {"set-prompt", R"(Usage: set-prompt <prompt>
Changes the prompt that appears at the beginning of each line. This does not apply
to the prompt used in help mode. It is highly recommended to use quotes if you want
whitespace in your prompt. Like with the other `set-` functions, you should put this
into ~/.config/cleo/startup to make it permanent.)"},
    {"defaults", R"(Cleo uses defaults for the music and playlist directories.
For your information, these are:
music:     ~/Music
playlists: ~/Music/playlists

These can be changed with `set-music` and `set-playlist` respectively.
When the initial setup is run, Cleo places commands to set these defaults in ~/.config/cleo/startup.
For more information about these files, see `run`.)"},
};

static constexpr int VOLUME_TOO_LOW{-1};
static constexpr int VOLUME_TOO_HIGH{-2};
static constexpr int REPEATS_TOO_LOW{-3};

std::string stem(std::string_view filename) { return fs::path{filename}.stem(); }

std::vector<std::string> transformStem(const std::vector<std::string>& input) {
    std::vector<std::string> output(input.size());
    std::transform(input.cbegin(), input.cend(), output.begin(), stem);
    return output;
}

void findHelp(const CommandDefinition& domain, const std::string& topic) {
    if (topic == "quit") {
        if (Threads::helpMode) {
            Threads::helpMode = false;
        } else {
            std::println("No help found for 'quit'.");
        }
        return;
    }
    if (domain.contains(topic)) {
        std::println("{}", domain.at(topic));
        return;
    }
    AutoMatch match{domain.keys(), topic};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("No help found for '{}'.", topic);
            break;
        case Match::ExactMatch:
            std::println("{}", domain.at(match.exactMatch()));
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
            break;
    }
}

void Cleo::play(Command& cmd) {
    if (cmd.argCount() != 1) {
        findHelp(Cleo::commandHelp, "play");
        return;
    }
    std::string song{cmd.nextArg()};
    fs::path songPath{Music::musicDir / song};
    if (fs::exists(songPath)) {
        if (Music::music.openFromFile(songPath)) {
            Music::curSong = song;
            Music::music.play();
        } else {
            std::println("The file is in an unsupported format.");
        }
        return;
    }
    AutoMatch match{Music::songs, song};
    std::string matchedSong{};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            return;
        case Match::ExactMatch:
            matchedSong = match.exactMatch();
            break;
        case Match::MultipleMatch:
            std::vector<std::string> baseSongNames{transformStem(match.matches)};
            std::println("Multiple matches found, could be one of {}.", join(baseSongNames, ", "));
            return;
    }
    if (!Music::music.openFromFile(Music::musicDir / matchedSong)) {
        std::println("A match was found, but the file is in an unsupported format.");
        return;
    }
    Music::curSong = stem(matchedSong);
    Music::repeats = 0;
    Music::music.play();
}

void Cleo::list(Command&) {
    std::vector<std::string> directorySorted{transformStem(Music::songs)};
    std::println("{}", join(directorySorted, ", "));
}

void Cleo::stop(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Playing) {
        Music::inPlaylistMode = false;
        Music::music.stop();
        Music::curSong = "";
    } else {
        std::println("Nothing playing.");
    }
}

void Cleo::exit(Command&) { Threads::running = false; }

static void getVolume() {
    float curVolume{Music::music.getVolume()};
    std::println("Volume: {:.1f}%", curVolume);
}

static void setVolume(const std::string& volume) {
    float newVolume{};
    try {
        newVolume = std::stof(volume);
        if (newVolume < 0) {
            throw VOLUME_TOO_LOW;
        } else if (newVolume > 100) {
            throw VOLUME_TOO_HIGH;
        } else {
            Music::music.setVolume(newVolume);
        }
    } catch (const std::exception&) {
        std::println("Value given was not a number.");
        return;
    } catch (const int x) {
        if (x == VOLUME_TOO_LOW) {
            std::println("Volume cannot be below 0.");
        } else if (x == VOLUME_TOO_HIGH) {
            std::println("Volume cannot be above 100.");
        }
        return;
    }
}

void Cleo::volume(Command& cmd) {
    if (cmd.argCount() == 0) {
        getVolume();
    } else if (cmd.argCount() != 1) {
        findHelp(Cleo::commandHelp, "volume");
    } else {
        setVolume(cmd.nextArg());
    }
}

void Cleo::pause(Command&) {
    using Status = sf::Music::Status;
    Status curStatus{Music::music.getStatus()};
    switch (curStatus) {
        case Status::Playing:
            Music::music.pause();
            break;
        case Status::Paused:
            Music::music.play();
            break;
        case Status::Stopped:
            std::println("Cannot pause or unpause while music is stopped.");
            break;
    }
}

std::string join(const std::vector<std::string>& vec, std::string_view delim) {
    if (vec.size() == 0) {
        return "";
    }
    std::string joined{vec.at(0)};
    for (std::vector<std::string>::const_iterator it = vec.begin() + 1; it != vec.end(); ++it) {
        if (!(*it).empty()) {
            joined += delim;
            joined += *it;
        }
    }
    return joined;
}

static std::vector<std::string> split(const std::string& str, std::string_view delim) {
    std::size_t curPos{0};
    std::size_t endPos{0};
    std::size_t delimLen{delim.length()};
    std::string curToken{};
    std::vector<std::string> ret{};
    while ((endPos = str.find(delim, curPos)) != std::string::npos) {
        curToken = str.substr(curPos, endPos - curPos);
        curPos = endPos + delimLen;
        ret.push_back(curToken);
    }
    ret.push_back(str.substr(curPos));
    return ret;
}

void Cleo::help(Command& cmd) {
    if (cmd.argCount() == 0 && !Threads::helpMode) {
        std::println("Welcome to Cleo's interactive help utility.");
        std::println("Type `commands` to see the list of commands.");
        std::println("Type `quit` or CTRL-D to return to Cleo.");
        Threads::helpMode = true;
        return;
    }
    CommandDefinition domain{Cleo::commandHelp};
    std::vector<std::string> args{};
    std::string search{};
    if (Threads::helpMode) {
        search = cmd.function();
    } else {
        search = cmd.nextArg();
    }
    if (search == "quit") {
        Threads::helpMode = false;
        return;
    }
    AutoMatch match{domain.keys(), search};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("No help found for '{}'.", search);
            return;
        case Match::ExactMatch:
            if (match.exactMatch() == "playlist" && cmd.argCount() >= 1) {
                domain = Playlist::commandHelp;
            } else {
                args.push_back(match.exactMatch());
            }
            args.append_range(cmd.arguments());
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
            return;
    }
    std::string topic{join(args, " ")};
    findHelp(domain, topic);
}

std::string numAsTimestamp(int time) {
    auto [elapsedMins, elapsedSecs]{std::div(time, 60)};
    if (elapsedMins > 60) {
        int elapsedHours{elapsedMins / 60};
        elapsedMins %= 60;
        return std::format("{}:{:02}:{:02}", elapsedHours, elapsedMins, elapsedSecs);
    }
    return std::format("{}:{:02}", elapsedMins, elapsedSecs);
}

void Cleo::time(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Stopped) {
        std::println("Nothing playing.");
    } else {
        int timeElapsed{(int)Music::music.getPlayingOffset().asSeconds()};
        std::string elapsedTimestamp{numAsTimestamp(timeElapsed)};
        std::string remainingTimestamp{
            numAsTimestamp((int)Music::music.getDuration().asSeconds() - timeElapsed)};
        std::println("{} elapsed, {} remaining.", elapsedTimestamp, remainingTimestamp);
    }
}

void Cleo::loop(Command&) {
    Music::music.setLooping(!Music::music.isLooping());
    std::println("Looping: {}.", Music::music.isLooping() ? "enabled" : "disabled");
}

static bool setRepeats(const std::string& repeats) {
    int newRepeats{};
    try {
        newRepeats = std::stoi(repeats);
        if (newRepeats < 0) {
            throw REPEATS_TOO_LOW;
        }
        Music::repeats = newRepeats;
        return true;
    } catch (const std::exception&) {
        std::println("Repeats must be a number.");
        Music::repeats = 0;
        return false;
    } catch (const int) {
        std::println("Repeats must be at least 0.");
        Music::repeats = 0;
        return false;
    }
}

void Cleo::repeat(Command& cmd) {
    bool successful{true};
    if (Music::curSong == "") {
        std::println("Nothing playing.");
        return;
    }
    if (cmd.argCount() == 0) {
        Music::repeats = 1;
    } else {
        successful = setRepeats(cmd.nextArg());
    }
    if (successful) {
        std::println("{} will be repeated {} time{}.", Music::curSong, Music::repeats,
                     Music::repeats == 1 ? "" : "s");
    }
}

static bool playlistFromCsv(const fs::path& playlistPath, std::vector<std::string>& playlist) {
    std::ifstream file{playlistPath};
    std::string curItem{};
    while (std::getline(file, curItem, ',')) {
        if (file.eof()) {
            // Account for dos and unix line endings. This is mainly for compatibility with smp.
            if (curItem.ends_with("\r\n")) {
                curItem.erase(curItem.length() - 2, 2);
            } else if (curItem.ends_with("\n")) {
                curItem.erase(curItem.length() - 1, 1);
            } else {
                std::println("Unknown line ending encountered when parsing playlist.");
                return false;
            }
        }
        playlist.push_back(curItem);
    }
    file.close();
    return true;
}

static void renameInPlaylist(const fs::path& playlistPath, std::string_view song, std::string_view newName) {
    std::vector<std::string> playlist{};
    if (!playlistFromCsv(playlistPath, playlist)) {
        return;
    }
    std::ofstream file;
    std::replace(playlist.begin(), playlist.end(), song, newName);
    file.open(playlistPath);
    file << join(playlist, ",") << '\n';
    file.close();
}

static void renameSongInPlaylists(std::string_view song, std::string_view newName) {
    for (const auto& playlist : fs::directory_iterator{Music::playlistDir}) {
        renameInPlaylist(playlist.path(), song, newName);
    }
}

static void renamePair(std::string_view oldName, const std::string& newName) {
    AutoMatch match{Music::songs, oldName};
    fs::path songToRename;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch: {
            songToRename = Music::musicDir / match.exactMatch();
            fs::path renamedSong{newName + songToRename.extension().string()};
            fs::rename(songToRename, Music::musicDir / renamedSong);
            renameSongInPlaylists(match.exactMatch(), renamedSong.string());
            std::replace(Music::curPlaylist.begin(), Music::curPlaylist.end(), match.exactMatch(),
                         renamedSong.string());
            std::replace(Music::shuffledPlaylist.begin(), Music::shuffledPlaylist.end(), match.exactMatch(),
                         renamedSong.string());
            std::string baseOldName{songToRename.stem()};
            std::println("Renamed {} -> {}.", baseOldName, newName);
            break;
        }
        case Match::MultipleMatch:
            std::vector<std::string> baseNames{transformStem(match.matches)};
            std::println("Multiple matches found, could be one of {}.", join(baseNames, ", "));
            break;
    }
}

void Cleo::rename(Command& cmd) {
    if (cmd.argCount() & 1 || cmd.argCount() < 2) {
        findHelp(Cleo::commandHelp, "rename");
        return;
    }
    while (cmd.argCount() >= 2) {
        renamePair(cmd.nextArg(), cmd.nextArg());
    }
}

static void removeFromPlaylist(const fs::path& playlistPath, std::string_view song) {
    std::vector<std::string> playlist{};
    if (!playlistFromCsv(playlistPath, playlist)) {
        return;
    }
    std::ofstream outfile;
    if (std::erase(playlist, song)) {
        outfile.open(playlistPath);
        outfile << join(playlist, ",") << '\n';
    }
    outfile.close();
}

static void removeSongFromPlaylists(std::string_view song) {
    for (const auto& playlist : fs::directory_iterator{Music::playlistDir}) {
        removeFromPlaylist(playlist.path(), song);
    }
}

static void removeSong(std::string_view song) {
    AutoMatch match{Music::songs, song};
    fs::path songPath;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch: {
            fs::remove(Music::musicDir / match.exactMatch());
            std::erase(Music::curPlaylist, match.exactMatch());
            std::erase(Music::shuffledPlaylist, match.exactMatch());
            removeSongFromPlaylists(match.exactMatch());
            std::string baseDelName{stem(match.exactMatch())};
            std::println("Deleted {}.", baseDelName);
            break;
        }
        case Match::MultipleMatch:
            std::vector<std::string> baseNames{transformStem(match.matches)};
            std::println("Multiple matches found, could be one of {}.", join(baseNames, ", "));
            break;
    }
}

void Cleo::del(Command& cmd) {
    if (cmd.argCount() == 0) {
        findHelp(Cleo::commandHelp, "delete");
        return;
    }
    while (cmd.argCount() > 0) {
        removeSong(cmd.nextArg());
    }
}

void Cleo::playlist(Command& cmd) {
    if (cmd.argCount() == 0) {
        const std::vector<std::string>& playlist{getPlaylist()};
        std::vector<std::string> humanizedSongs{transformStem(playlist)};
        std::println("{}", join(humanizedSongs, ", "));
        return;
    }
    cmd.nextArg();
    parseCmd(cmd, Playlist::commands);
}

static int timestampAsNum(const std::string& timestamp) {
    static const std::regex timestampFormat{R"(^([0-9]?[1-9]:)?[0-5]?[0-9]:[0-5][0-9]$)"};
    int duration{};
    std::vector<std::string> timestampComponents{};
    constexpr int numTimestampComponents{3};
    if (std::regex_match(timestamp, timestampFormat)) {
        timestampComponents = split(timestamp, ":");
    } else {
        return -1;
    }
    std::reverse(timestampComponents.begin(), timestampComponents.end());
    timestampComponents.resize(numTimestampComponents); // 3 integers representing hours, minutes, and seconds
    for (std::size_t i{0}; i < numTimestampComponents; ++i) {
        if (!timestampComponents[i].empty()) {
            duration += std::stoi(timestampComponents[i]) * (int)std::pow(60, i);
        }
    }
    return duration;
}

static sf::Time getTime(Command& cmd) {
    std::size_t length{};
    std::string time{cmd.nextArg()};
    int timestamp{};
    try {
        timestamp = std::stoi(time, &length);
        if (length != time.length()) {
            throw std::exception{};
        }
    } catch (const std::exception&) {
        timestamp = timestampAsNum(time);
    }
    return sf::seconds((float)timestamp);
}

static void seekRelative(Command& cmd, bool forward) {
    if (cmd.argCount() != 1) {
        findHelp(Cleo::commandHelp, forward ? "forward" : "rewind");
        return;
    }
    sf::Time duration{getTime(cmd)};
    if (duration.asSeconds() < 0) {
        std::println("Invalid duration or timestamp given. See 'help timestamp' for more.");
        return;
    }
    sf::Time curOffset{Music::music.getPlayingOffset()};
    if (forward) {
        Music::music.setPlayingOffset(curOffset + duration);
    } else {
        if ((curOffset - duration).asSeconds() < 0) {
            Music::music.setPlayingOffset(sf::Time::Zero);
        } else {
            Music::music.setPlayingOffset(curOffset - duration);
        }
    }
}

void Cleo::seek(Command& cmd) {
    if (cmd.argCount() != 1) {
        findHelp(Cleo::commandHelp, "seek");
        return;
    }
    if (Music::music.getStatus() == sf::Music::Status::Stopped) {
        std::println("Nothing playing.");
        return;
    }
    sf::Time offset{getTime(cmd)};
    if (offset.asSeconds() < 0) {
        std::println("Invalid duration or timestamp given. See 'help timestamp' for more.");
        return;
    }
    Music::music.setPlayingOffset(offset);
}

void Cleo::forward(Command& cmd) { seekRelative(cmd, true); }

void Cleo::rewind(Command& cmd) { seekRelative(cmd, false); }

static void findSong(std::string_view substr) {
    std::vector<std::string> matches{};
    std::for_each(Music::songs.begin(), Music::songs.end(), [&](std::string song) {
        if (song.starts_with(substr)) {
            matches.push_back(stem(song));
        }
    });
    std::println("{}: {}", substr, join(matches, ", "));
}

void Cleo::find(Command& cmd) {
    if (cmd.argCount() == 0) {
        findHelp(Cleo::commandHelp, "find");
        return;
    }
    while (cmd.argCount() > 0) {
        findSong(cmd.nextArg());
    }
}

void Cleo::setMusicDir(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Music directory: {}", Music::musicDir.string());
        return;
    }
    std::string tmpDir{cmd.nextArg()};
    wordexp_t p;
    wordexp(tmpDir.c_str(), &p, 0);
    fs::path newMusicDir{p.we_wordv[p.we_offs]};
    // Expand ~ to home directory
    wordfree(&p);
    if (!fs::exists(newMusicDir)) {
        std::println("Music directory {} does not exist.", newMusicDir.string());
        return;
    }
    Music::musicDir = newMusicDir;
    updateSongs();
}

void Cleo::setPlaylistDir(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Playlist directory: {}", Music::playlistDir.string());
        return;
    }
    std::string tmpDir{cmd.nextArg()};
    wordexp_t p;
    wordexp(tmpDir.c_str(), &p, 0);
    fs::path newPlaylistDir{p.we_wordv[p.we_offs]};
    wordfree(&p);
    if (!fs::exists(newPlaylistDir)) {
        std::println("Playlist directory {} does not exist.", newPlaylistDir.string());
        return;
    }
    Music::playlistDir = newPlaylistDir;
    updatePlaylists();
}

void Cleo::setPrompt(Command& cmd) {
    if (cmd.argCount() != 1) {
        findHelp(Cleo::commandHelp, "set-prompt");
        return;
    }
    Music::prompt = cmd.nextArg();
}

static void runScript(std::string&& script) {
    AutoMatch match{Music::scripts, script};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Script not found.");
            return;
        case Match::ExactMatch:
            script = match.exactMatch();
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", join(match.matches, ", "));
            return;
    }
    Music::isExecutingScript = true;
    std::ifstream scriptPath{Music::scriptDir / script};
    std::string line{};
    while (std::getline(scriptPath, line)) {
        if (!line.starts_with("#")) {
            executeCmds(parseString(line));
        }
    }
    Music::isExecutingScript = false;
}

void Cleo::run(Command& cmd) {
    if (cmd.argCount() == 0) {
        findHelp(Cleo::commandHelp, "run");
        return;
    }
    if (Music::isExecutingScript) {
        std::println("Cannot run a script while executing another script.");
        return;
    }
    while (cmd.argCount() > 0) {
        runScript(cmd.nextArg());
    }
}
