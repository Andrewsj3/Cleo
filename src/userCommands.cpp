#include "userCommands.hpp"
#include "autocomplete.hpp"
#include "command.hpp"
#include "music.hpp"
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
namespace fs = std::filesystem;
static const std::vector<std::string> commands{
    "delete", "exit",   "help",   "list", "loop", "pause",
    "play",   "rename", "repeat", "stop", "time", "volume",
};
static const CommandDefinition programHelp{
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
    {"commands", join(commands, "\n")},
    {"time", "Shows the current song's elapsed time and remaining time."},
    {"loop", "Toggles whether songs should loop when they reach the end."},
    {"repeat", R"(Usage: repeat [numRepeats]
By default, repeats the song once.
Otherwise, repeats the song the given number of times provided it is at least 0.)"},
    {"rename", R"(Usage: rename <oldName> <newName>
Renames a song in the music directory (autocomplete is supported). Note the song
must be in a supported format (see 'help formats') or it will fail. THIS DOES NOT
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
     R"(When typing a song or command name, you can type the first few characters
as long as it doesn't match anything else, e.g.
`l` doesn't work because it matches both `list` and `loop`.
`li` works because it only matches list.)"}};
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
    // check for exact name match not including extension
    MusicMatch match{autocomplete(Music::songs, song)};
    std::string matchedSong{};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            return;
        case Match::ExactMatch:
            matchedSong = match.matches.at(0);
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.",
                         join(match.matches, ", "));
            return;
    }
    bool foundSong{false};
    for (const auto& ext : Music::supportedExtensions) {
        if (Music::load.openFromFile(Music::musicDir / (matchedSong + ext))) {
            (void)Music::music.openFromFile(Music::musicDir / (matchedSong + ext));
            foundSong = true;
            Music::curSong = matchedSong;
            break;
        }
    }
    if (!foundSong) {
        std::println("A match was found, but the file is in an unsupported format.");
        return;
    }
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
    std::string dirList{join(directorySorted, ", ")};
    std::println("{}", dirList);
}

void Cleo::stop(Command&) {
    if (Music::music.getStatus() == sf::Music::Status::Playing) {
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

void findHelp(const std::string& topic) {
    if (topic == "quit") {
        if (Threads::helpMode) {
            Threads::helpMode = false;
        } else {
            std::println("No help found for 'quit'.");
        }
        return;
    }

    MusicMatch match{autocomplete(programHelp.keys(), topic)};
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("No help found for '{}'.", topic);
            break;
        case Match::ExactMatch:
            std::println("{}", programHelp.at(match.exactMatch()));
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
    std::string topic{};
    if (Threads::helpMode) {
        std::vector<std::string> args{cmd.function()};
        args.append_range(cmd.arguments());
        topic = join(args, " ");
    } else {
        topic = join(cmd.arguments(), " ");
    }
    findHelp(topic);
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
    MusicMatch match{autocomplete(Music::songs, oldName)};
    fs::path songToRename;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch:
            for (const auto& ext : Music::supportedExtensions) {
                if (fs::exists(Music::musicDir / (match.exactMatch() + ext))) {
                    songToRename = Music::musicDir / (match.exactMatch() + ext);
                    fs::rename(songToRename, Music::musicDir / (newName + ext));
                    std::println("Renamed {} -> {}.", match.exactMatch(), newName);
                    return;
                }
            }
            std::println("A match was found, but the file is in an unsupported format.");
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", match.matches);
            break;
    }
}

void Cleo::del(Command& cmd) {
    if (cmd.argCount() != 1) {
        std::println("Expected exactly one song to delete.");
        return;
    }
    std::string songToDelete{cmd.nextArg()};
    MusicMatch match{autocomplete(Music::songs, songToDelete)};
    fs::path songPath;
    switch (match.matchType) {
        case Match::NoMatch:
            std::println("Song not found.");
            break;
        case Match::ExactMatch:
            for (const auto& ext : Music::supportedExtensions) {
                if (fs::exists(Music::musicDir / (match.exactMatch() + ext))) {
                    fs::remove(Music::musicDir / (match.exactMatch() + ext));
                    std::println("Deleted {}.", match.exactMatch());
                    return;
                }
            }
            std::println("A match was found, but the file is in an unsupported format.");
            break;
        case Match::MultipleMatch:
            std::println("Multiple matches found, could be one of {}.", match.matches);
            break;
    }
}
