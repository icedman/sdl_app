#ifndef STYLE_H
#define STYLE_H

#include "renderer.h"
#include "theme.h"

#include <string>

struct view_style_t {
    std::string class_name;
    std::string font;
    bool italic;
    bool bold;

    color_info_t fg;
    color_info_t bg;
    color_info_t border_color;
    
    bool filled;
    int border_width;
    int corner_radius;

    bool inherit;
};

void view_style_register(view_style_t style, std::string name);
view_style_t view_style_get(std::string name);

#endif // STYLE_H
