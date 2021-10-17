#ifndef RECT_H
#define RECT_H

struct point_t {
    int x;
    int y;
};

struct rect_t {
    int x;
    int y;
    int w;
    int h;
};

bool rects_overlap(rect_t a, rect_t b);
bool rects_equal(rect_t a, rect_t b);
bool point_in_rect(point_t p, rect_t r);
rect_t intersect_rects(rect_t a, rect_t b);
rect_t merge_rects(rect_t a, rect_t b);

#endif RECT_H