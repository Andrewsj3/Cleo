#include "music.hpp"
#include "threads.hpp"
#include <print>
#include <sys/inotify.h>
#include <thread>

void monitorChanges() {
    using namespace std::chrono_literals;
    int fd{inotify_init1(IN_NONBLOCK)};
    // We need nonblocking otherwise the read call below may never terminate
    if (fd == -1) {
        std::println(
            "ERROR: Could not initialize inotify for directory monitoring. Cleo will not track changes in "
            "the music directory.");
        return;
    }
    int wdMusic{};
    int wdPlaylist{};
    std::uint32_t musicEvents{IN_CREATE | IN_DELETE | IN_MOVE};
    std::uint32_t playlistEvents{IN_CREATE | IN_DELETE};
    std::filesystem::path musicDir{Music::musicDir};
    std::filesystem::path playlistDir{Music::playlistDir};
    if ((wdMusic = inotify_add_watch(fd, Music::musicDir.c_str(), musicEvents)) == -1) {
        std::println("ERROR: Could not track music directory. Songs will not be updated.");
    }
    if ((wdPlaylist = inotify_add_watch(fd, Music::playlistDir.c_str(), playlistEvents)) == -1) {
        std::println("ERROR: Could not track playlist directory. Playlists will not be updated.");
    }
    char buf[sizeof(inotify_event) + NAME_MAX + 1];
    inotify_event* event{};
    ssize_t size{};
    while (true) {
        if (!Threads::running) [[unlikely]] {
            return;
        }
        std::this_thread::sleep_for(30ms);
        if (musicDir != Music::musicDir) {
            // Music directory can change during the course of the program, so we need to keep track
            inotify_rm_watch(fd, wdMusic);
            wdMusic = inotify_add_watch(fd, Music::musicDir.c_str(), musicEvents);
            musicDir = Music::musicDir;
        }
        if (playlistDir != Music::playlistDir) {
            inotify_rm_watch(fd, wdPlaylist);
            wdPlaylist = inotify_add_watch(fd, Music::playlistDir.c_str(), playlistEvents);
            playlistDir = Music::playlistDir;
        }
        size = read(fd, buf, sizeof(buf));
        if (size <= 0) {
            continue;
        }
        for (char* ptr = buf; ptr < buf + size; ptr += sizeof(inotify_event) + event->len) {
            event = (inotify_event*)ptr;
            if (event->mask & musicEvents) {
                if (event->wd == wdMusic) {
                    updateSongs();
                } else if (event->wd == wdPlaylist) {
                    updatePlaylists();
                } else {
                    assert(0 && "Watch descriptor did not match music or playlist directory.");
                }
            }
        }
    }
    inotify_rm_watch(fd, wdMusic);
    inotify_rm_watch(fd, wdPlaylist);
}
