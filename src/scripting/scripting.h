#ifndef SCRIPTING_H
#define SCRIPTING_H

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