#include "backend.h"

#include <chrono>

static struct backend_t global_backend;
static int start_millis = 0;

struct backend_t* backend_t::instance()
{
    return &global_backend;
}

backend_t::backend_t()
{
}

void backend_t::setClipboard(std::string text)
{
    cliptext = text;
}

std::string backend_t::getClipboard()
{
    return cliptext;
}

int backend_t::now()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}

void backend_t::begin()
{
    start_millis = now();
}

int backend_t::elapsed()
{
    return now() - start_millis;
}

void backend_t::delay(int ms)
{
    struct timespec waittime;

    waittime.tv_sec = (ms / 1000);
    ms = ms % 1000;
    waittime.tv_nsec = ms * 1000 * 1000;

    nanosleep(&waittime, NULL);
}
