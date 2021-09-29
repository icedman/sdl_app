#include "backend.h"

#include <chrono>

static struct backend_t* backendInstance = 0;
static int last_millis = 0;

struct backend_t* backend_t::instance()
{
    return backendInstance;
}

backend_t::backend_t()
{
    backendInstance = this;
}

int backend_t::ticks()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    auto elapsed = last_millis > 0 ? millis - last_millis : 1;
    if (elapsed > 0) {
        last_millis = millis;
    }
    return elapsed;
}