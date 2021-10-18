#include "damage.h"
#include "system.h"

damage_t global_damage;

damage_t* damage_t::instance()
{
    return &global_damage;
}

void damage_t::damage_whole()
{
    damage_rects.clear();
    damage({ 0, 0, system_t::instance()->renderer.width(), system_t::instance()->renderer.height() });
}

void damage_t::damage(rect_t r)
{
    if (r.x < 0) {
        r.w += r.x;
        r.x = 0;
    }
    if (r.y < 0) {
        r.h += r.y;
        r.y = 0;
    }
    if (r.w < 0 || r.h < 0)
        return;
    damage_rects.push_back(r);
}

bool damage_t::should_render(rect_t rect)
{
    for (auto d : damage_rects) {
        if (rects_overlap(rect, d)) {
            return true;
        }
    }
    return false;
}

void damage_t::clear()
{
    damage_rects.clear();
}

rect_t* damage_t::rects()
{
    if (!damage_rects.size()) {
        return NULL;
    }
    return &damage_rects[0];
}

int damage_t::count()
{
    return damage_rects.size();
}
