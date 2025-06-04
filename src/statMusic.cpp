#include "music.hpp"
#include "threads.hpp"
#include <filesystem>
#include <thread>
void monitorChanges() {
    using namespace std::chrono_literals;
    std::filesystem::file_time_type mtimeOld{std::filesystem::last_write_time(Music::musicDir)};
    while (true) {
        if (!Threads::running) [[unlikely]] {
            return;
        }
        std::this_thread::sleep_for(5s);
        std::filesystem::file_time_type mtimeNew{
            std::filesystem::last_write_time(Music::musicDir)};
        if (mtimeNew > mtimeOld) [[unlikely]] {
            mtimeOld = mtimeNew;
            updateSongs();
        }
    }
}
