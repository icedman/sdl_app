#ifndef DAMAGE_H
#define DAMAGE_H

#include "renderer.h"
#include <vector>

struct damage_t
{
    static damage_t* instance();
    
    void damage(RenRect rect);
    void reset();
    std::vector<RenRect> damage_rects;
};

#endif // DAMAGE_H