#ifndef ANIMATION_H
#define ANIMATION_H

#include "view.h"

#include <algorithm>
#include <functional>
#include <vector>

struct animation;
typedef std::function<bool(animation* anim)> animation_callback_t;

struct animation {

    animation();

    int delay;
    int length;
    int index;
    bool running;
    bool loop;

    virtual void update(int tick);

    void run(int index = 0);
    void pause();
    void cancel();
    int pos();
    bool is_running();

    virtual float value();

    static void request_animation();
    static bool has_animations();

    animation_callback_t callback;
};

typedef std::shared_ptr<animation> animation_ptr;

struct animate_ease_values : animation {

    void update(int tick) override;
    void run(float start, float end, float duration);
    float value() override;

    float start;
    float end;
};

#endif // ANIMATION_H