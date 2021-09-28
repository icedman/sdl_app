#include "animation.h"
#include "Cubic.h"

animation::animation()
    : delay(0)
    , length(0)
    , index(0)
    , loop(false)
    , running(false)
{
}

void animation::update(int tick)
{
    if (!is_running())
        return;
    index += tick;

    if (index >= length) {
        running = loop;
        index = length;
    }

    if (is_running()) {
        request_animation();
    }
}

bool animation::is_running()
{
    return running;
}

void animation::run(int idx)
{
    index = idx;
    running = true;
}

void animation::pause()
{
    running = false;
}

void animation::cancel()
{
    index = 0;
    running = false;
}

int animation::pos()
{
    return index;
}

float animation::value()
{
    return 0;
}

static bool _animated = false;
void animation::request_animation()
{
    _animated = true;
}

bool animation::has_animations()
{
    bool res = _animated;
    _animated = false;
    return res;
}

void animate_ease_values::run(float s, float e, float duration)
{
    start = s;
    end = e;
    length = duration;
    animation::run();
}

float animate_ease_values::value()
{
    if (!is_running()) {
        return 0;
    }

    float t = (float)index / length;
    float d = (float)(end - start) / length;
    float v = Cubic::easeIn(t, start, d, length);
    return v;
}

void animate_ease_values::update(int tick)
{
    if (is_running()) {
        animation::update(tick);
        if (callback) {
            callback(this);
        }
    }
}