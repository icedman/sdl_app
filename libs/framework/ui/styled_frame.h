#ifndef VIEW_STYLE_H
#define VIEW_STYLE_H

#include "color.h"
#include "rect.h"
#include <cstdint>

struct renderer_t;

struct borders_t {
    uint8_t width;
    uint8_t left;
    uint8_t top;
    uint8_t right;
    uint8_t bottom;
};

struct styled_frame_t {
    // colors
    color_t fg;
    color_t bg;

    // text
    bool italic;
    bool bold;
    bool underline;

    // background
    bool filled;

    // borders
    borders_t border;
    borders_t border_radius;
    color_t border_color;

    // margins
    borders_t margin;
    borders_t padding;

    bool available;
};

void render_styled_frame(renderer_t* renderer, rect_t rect, styled_frame_t& style);

#endif // VIEW_STYLE_H