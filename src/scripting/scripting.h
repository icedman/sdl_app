#ifndef SCRIPTING_H
#define SCRIPTING_H

#ifdef ENABLE_SCRIPTING
extern "C" {
#include "quickjs-libc.h"
#include "quickjs.h"
}
#endif

#include <string>

struct Scripting {
    static Scripting* instance();

    bool init();
    void shutdown();
    void update(int ticks);
    void restart();

    int execute(std::string script);
};

#endif // SCRIPTING_H