#ifndef FONT_H
#define FONT_H

#include <functional>
#include <string>

#include "color.h"

struct renderer_t;
struct font_t {
    int width;
    int height;
    float size;
    std::string name;
    std::string desc;
    std::string path;
    std::string alias;

    int ref;

    std::function<int(renderer_t* renderer, font_t* font, char* text, int x, int y, color_t clr, bool bold, bool italic, bool underline)> draw_text;
    std::function<void(font_t*)> destroy;
};

#endif FONT_H