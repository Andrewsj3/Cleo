#include "music.hpp"
#include "threads.hpp"
#include <filesystem>
#include <flat_map>
#include <thread>

namespace fs = std::filesystem;
static void addWatchPath(std::flat_map<fs::path, fs::file_time_type>& directoryWatch, const fs::path& path) {
    directoryWatch.emplace(path, fs::last_write_time(path));
}

void monitorChanges() {
    using namespace std::chrono_literals;
    std::flat_map<fs::path, fs::file_time_type> directoryWatch{};
    addWatchPath(directoryWatch, Music::musicDir);
    addWatchPath(directoryWatch, Music::playlistDir);
    while (true) {
        if (!Threads::running) [[unlikely]] {
            return;
        }
        std::this_thread::sleep_for(5s);
        // The content of these directories will not change often, so we only need to check once
        // in a while. This sleep call is also why we have to detach this thread, since we would
        // have to wait up to 5 seconds otherwise
        for (const auto& dir : directoryWatch) {
            fs::file_time_type mtimeNew{fs::last_write_time(dir.first)};
            if (mtimeNew > dir.second) {
                dir.second = mtimeNew;
                if (dir.first == Music::musicDir) {
                    updateSongs();
                } else {
                    updatePlaylists();
                }
            }
        }
    }
}
