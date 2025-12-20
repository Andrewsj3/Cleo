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
    std::thread backgroundThreadObj{backgroundThread};
    std::thread inputThreadObj{inputThread};

    statThreadObj.join();
    inputThreadObj.join();
    backgroundThreadObj.join();
}
