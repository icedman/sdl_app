#include "statusbar.h"
#include "app.h"

static struct statusbar_t* statusbarInstance = 0;

statusbar_t::statusbar_t()
    : frames(0)
{
    statusbarInstance = this;
}

struct statusbar_t* statusbar_t::instance()
{
    return statusbarInstance;
}

void statusbar_t::setText(std::string s, int pos, int size)
{
    if (pos < 0) {
        pos = 5 + pos;
    }
    sizes[pos] = size;
    text[pos] = s;

    // printf("%d %s\n", pos, s.c_str());
}

void statusbar_t::setStatus(std::string s, int f)
{
    status = s;
    frames = (f + 500);
}

void statusbar_t::update(int delta)
{
    if (frames < 0) {
        return;
    }

    frames = frames - delta;
    if (frames < 500) {
        status = "";
    }
}
