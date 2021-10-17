#ifndef DAMAGE_H
#define DAMAGE_H

#include "rect.h"
#include <vector>

struct damage_t {

    static damage_t* instance();

    void damage_whole();
    void damage(rect_t r);
    void clear();
    rect_t* rects();
    int count();

    bool should_render(rect_t rect);

    std::vector<rect_t> damage_rects;
};

#endif DAMAGE_H