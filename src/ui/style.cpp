#include "style.h"

#include <algorithm>
#include <map>

std::map<std::string, view_style_t> styleMap;

void view_style_clear(view_style_t& style)
{
    view_style_t vs_default = {
        font : "",
        italic : false,
        bold : false,
        underline : false,
    };

    vs_default.fg = { 0,0,0,0 };
    vs_default.bg = { 0,0,0,0 };
    vs_default.filled = false;

    vs_default.border_color = { 0,0,0,0 };
    memset(&vs_default.border, 0, sizeof(borders_t));
    memset(&vs_default.border_radius, 0, sizeof(borders_t));
    memset(&vs_default.margin, 0, sizeof(borders_t));
    memset(&vs_default.padding, 0, sizeof(borders_t));

    style = vs_default;
}

void view_style_register(view_style_t style, std::string name)
{
    styleMap[name] = style;
}

view_style_t view_style_get(std::string name)
{
    if (styleMap.find(name) == styleMap.end()) {
        name = "default";
    }
    return styleMap[name];
}