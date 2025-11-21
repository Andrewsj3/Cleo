#include "threads.hpp"
#include "input.hpp"
#include "statMusic.hpp"
#include <string>
#include <thread>

namespace Threads {
    std::string userInput{};
    bool running{true};
    bool readyForInput{true};
    bool helpMode{false};
} // namespace Threads

void runThreads() {
    std::thread statThreadObj{monitorChanges};
    std::thread inputThreadObj{inputThread};
    std::thread backgroundThreadObj{backgroundThread};

    statThreadObj.detach();
    // Detach is needed for program to exit immediately at user's request
    inputThreadObj.join();
    backgroundThreadObj.join();
}
