#include "damage.h"

static damage_t dm;

damage_t* damage_t::instance()
{
    return &dm;
}

void damage_t::damage(RenRect rect)
{
    if (Renderer::instance()->is_terminal()) {
        damage_rects.clear();
        damage_rects.push_back({ 0, 0, 1000, 1000 });
        return;
    }
    damage_rects.push_back(rect);
}

void damage_t::reset()
{
    damage_rects.clear();
}