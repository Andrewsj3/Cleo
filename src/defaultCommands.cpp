#include "defaultCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "input.hpp"
#include "music.hpp"
#include "playlistCommands.hpp"
#include "threads.hpp"
#include <SFML/Audio/Music.hpp>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <flat_map>
#include <print>
#include <string>
#include <string_view>
#include <vector>
using CommandDefinition = std::flat_map<std::string, std::string>;
using CommandMap = std::flat_map<std::string, std::function<void(Command&)>>;
// Note this definition means each command has the same signature, even though some commands
// don't need arguments

namespace fs = std::filesystem;
const CommandMap Cleo::commands{
    {"exit", Cleo::exit},         {"list", Cleo::list},      {"pause", Cleo::pause},
    {"play", Cleo::play},         {"stop", Cleo::stop},      {"volume", Cleo::volume},
    {"help", Cleo::help},         {"time", Cleo::time},      {"loop", Cleo::loop},
    {"repeat", Cleo::repeat},     {"rename", Cleo::rename},  {"delete", Cleo::del},
    {"playlist", Cleo::playlist}, {"queue", Cleo::playlist},
};
const std::vector<std::string> Cleo::commandList{
    "delete",   "exit",   "help",   "list", "loop", "pause",  "play",
    "playlist", "rename", "repeat", "stop", "time", "volume",
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
With no arguments, shows the current volume.
Otherwise, sets the new volume provided it is between 0 and 100.)"},
    {"help", "Shows how commands work and how you can use Cleo."},
    {"commands", join(Cleo::commandList, "\n")},
    {"time", "Shows the current song's elapsed time and remaining time."},
    {"loop", "Toggles whether songs should loop when they reach the end."},
    {"repeat", R"(Usage: repeat [numRepeats]
By default, repeats the song once.
Otherwise, repeats the song the given number of times provided it is at least 0.)"},
    {"rename", R"(Usage: rename <oldName> <newName>
Renames a song in the music directory (autocomplete is supported). Note the song
must be in a supported format (see 'help formats') or it will fail. The new song will have
the same extension as the original, so don't add an extension yourself. THIS DOES NOT
CHECK IF A SONG WILL BE OVERWRITTEN.)"},
    {"formats", R"(Supported formats:
mp3
ogg
flac
wav
aiff)"},
    {"delete", R"(Usage: delete <songName>
Deletes a song from the music directory. Like `rename`, the song must be in a
supported format or it will not be deleted.)"},
    {"autocomplete",
     R"(When typing a song, file, or command, you can type the first few characters
as long as it doesn't match anything else, e.g.
`l` doesn't work because it matches both `list` and `loop`.
`li` works because it only matches list.)"},
    {"playlist", R"(Usage: playlist [subcommand] [argument]
This allows you to interact with the playlist in various ways.
If no subcommand is specified, it will show all songs in the playlist.
Do `playlist commands` to see all subcommands or `playlist <subcommand>` to see more
specific help. Note: you can also use the alias `queue` to make autocompletion easier.)"}};

static constexpr int VOLUME_TOO_LOW{-1};
static constexpr int VOLUME_TOO_HIGH{-2};
static constexpr int REPEATS_TOO_LOW{-3};

void Cleo::play(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected exactly one song to play.");
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
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            return;
    }
    if (!Music::music.openFromFile(Music::musicDir / matchedSong)) {
        std::println("A match was found, but the file is in an unsupported format.");
        return;
    }
    Music::curSong = fs::path(matchedSong).stem();
    Music::repeats = 0;
    Music::music.play();
}

void Cleo::list(Command&) {
    std::stringstream sb{};
    std::vector<std::string> directorySorted{};
    for (const auto& dirEntry : fs::directory_iterator(Music::musicDir)) {
        std::string ext{dirEntry.path().extension()};
        if (Music::supportedExtensions.contains(ext)) {
            directorySorted.push_back(dirEntry.path().stem().string());
        }
    }
    std::sort(directorySorted.begin(), directorySorted.end());
    // Print in alphabetical order for consistency
    std::string dirList{join(directorySorted, ", ")};
    std::println("{}", dirList);
}

void Cleo::stop(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Playing) {
        Music::inPlaylistMode = false;
        Music::music.stop();
    } else {
        std::println("Nothing playing.");
    }
}

void Cleo::exit(Command&) { Threads::running = false; }

void getVolume() {
    float curVolume{Music::music.getVolume()};
    std::println("Volume: {:.1f}%", curVolume);
}

void setVolume(const std::string& volume) {
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
    } else if (vec.size() == 1) {
        return vec.at(0);
    }
    std::string joined{};
    for (std::vector<std::string>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
        joined += *it;
        if (it != vec.end() - 1) {
            joined += delim;
        }
    }
    return joined;
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
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            break;
    }
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
    AutoMatch match{domain.keys(), search};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("No help found for '{}'", search);
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
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            return;
    }
    std::string topic{join(args, " ")};
    findHelp(domain, topic);
}

void Cleo::time(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Stopped) {
        std::println("Nothing playing.");
    } else {
        int timeElapsed{(int)Music::music.getPlayingOffset().asSeconds()};
        auto [elapsedMins, elapsedSecs]{std::div(timeElapsed, 60)};
        int remaining{(int)Music::music.getDuration().asSeconds() - timeElapsed};
        auto [remainingMins, remainingSecs]{std::div(remaining, 60)};
        std::println("{0:}:{1:02} elapsed, {2:}:{3:02} remaining.", elapsedMins, elapsedSecs,
                     remainingMins, remainingSecs);
    }
}

void Cleo::loop(Command&) {
    Music::music.setLooping(!Music::music.isLooping());
    std::println("Looping: {}.", Music::music.isLooping() ? "enabled" : "disabled");
}

bool setRepeats(const std::string& repeats) {
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

void Cleo::rename(Command& cmd) {
    if (cmd.argCount() != 2) {
        std::println("Expected an old name and a new name.");
        return;
    }
    std::string oldName{cmd.nextArg()};
    std::string newName{cmd.nextArg()};
    AutoMatch match{Music::songs, oldName};
    fs::path songToRename;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch: {
            songToRename = Music::musicDir / match.exactMatch();
            fs::rename(songToRename,
                       Music::musicDir / (newName + songToRename.extension().string()));
            std::string baseOldName{fs::path(songToRename).stem()};
            std::println("Renamed {} -> {}.", baseOldName, newName);
            break;
        }
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            break;
    }
}

void Cleo::del(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected exactly one song to delete.");
        return;
    }
    std::string songToDelete{cmd.nextArg()};
    AutoMatch match{Music::songs, songToDelete};
    fs::path songPath;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch: {
            fs::remove(Music::musicDir / match.exactMatch());
            std::string baseDelName{fs::path(match.exactMatch()).stem()};
            std::println("Deleted {}.", baseDelName);
            break;
        }
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            break;
    }
}

void Cleo::playlist(Command& cmd) {
    if (cmd.arguments().empty()) {
        std::vector<std::string> humanizedSongs{};
        humanizedSongs.resize(Music::curPlaylist.size());
        std::transform(Music::curPlaylist.cbegin(), Music::curPlaylist.cend(),
                       humanizedSongs.begin(),
                       [](const std::string& song) { return fs::path(song).stem(); });
        // We store the extensions to make loading songs easier, but we don't want to show that
        // to the user
        std::println("{}", join(humanizedSongs, ", "));
        return;
    }
    cmd.nextArg();
    parseCmd(cmd, Playlist::commands);
}
