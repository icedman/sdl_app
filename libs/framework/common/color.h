#ifndef COLOR_H
#define COLOR_H

struct color_t {
    int r;
    int g;
    int b;
    int a;
};

inline bool color_is_set(color_t clr) { return clr.r > -1 && (clr.r != 0 || clr.g != 0 || clr.b != 0 || clr.a != 0); }
color_t color_darker(color_t clr, int amount);
color_t color_lighter(color_t clr, int amount);
bool color_is_dark(color_t clr);

#endif COLOR_H