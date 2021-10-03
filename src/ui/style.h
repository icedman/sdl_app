#ifndef STYLE_H
#define STYLE_H

#include "renderer.h"
#include "theme.h"

#include <string>

struct borders_t
{
    uint8_t width;
    uint8_t left;
    uint8_t top;
    uint8_t right;
    uint8_t bottom;
};

struct view_style_t {
    std::string class_name;

    // text
    std::string font;
    bool italic;
    bool bold;
    bool underline;

    // colors
    color_info_t fg;
    color_info_t bg;

    // background
    bool filled;

    // borders
    color_info_t border_color;
    borders_t border;
    borders_t border_radius;
    borders_t margin;
    borders_t padding;
};

void style_init();
void style_clear(view_style_t& style);
void style_register(view_style_t style, std::string name);
view_style_t style_get(std::string name);

#endif // STYLE_H
