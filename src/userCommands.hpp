#pragma once

#include "command.hpp"
namespace Cleo {
    void play(Command&);
    void stop(Command&);
    void volume(Command&);
    void pause(Command&);
    void exit(Command&); // exit is treated slightly differently, this only exists to placate
    // parseCmd
} // namespace Cleo
