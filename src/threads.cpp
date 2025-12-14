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

#if defined(__unix)
    statThreadObj.join();
#else
    statThreadObj.detach();
#endif
    inputThreadObj.join();
    backgroundThreadObj.join();
}
