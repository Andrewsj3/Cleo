#pragma once

#include "command.hpp"
namespace Cleo {
    void help(Command&);
    void play(Command&);
    void list(Command&);
    void stop(Command&);
    void volume(Command&);
    void pause(Command&);
    void exit(Command&);
} // namespace Cleo
