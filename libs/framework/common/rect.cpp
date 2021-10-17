#include "rect.h"

bool rects_overlap(rect_t a, rect_t b)
{
    if (a.x >= b.x + b.w || b.x >= a.x + a.w | a.y >= b.y + b.h || b.y >= a.y + a.h) {
        return false;
    }

    return true;
}

bool point_in_rect(point_t p, rect_t r)
{
    return p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h;
}

static inline int min(int a, int b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }

rect_t intersect_rects(rect_t a, rect_t b)
{
    int x1 = max(a.x, b.x);
    int y1 = max(a.y, b.y);
    int x2 = min(a.x + a.w, b.x + b.w);
    int y2 = min(a.y + a.h, b.y + b.h);
    return (rect_t){ x1, y1, max(0, x2 - x1), max(0, y2 - y1) };
}

rect_t merge_rects(rect_t a, rect_t b)
{
    int x1 = min(a.x, b.x);
    int y1 = min(a.y, b.y);
    int x2 = max(a.x + a.w, b.x + b.w);
    int y2 = max(a.y + a.h, b.y + b.h);
    return (rect_t){ x1, y1, x2 - x1, y2 - y1 };
}

bool rects_equal(rect_t a, rect_t b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}