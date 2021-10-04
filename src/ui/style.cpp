#include "style.h"

#include <algorithm>
#include <map>

std::map<std::string, view_style_t> styleMap;

void style_init()
{
    view_style_t vs_default;
    style_clear(vs_default);

    vs_default.fg = { 250, 250, 250, 255 };
    vs_default.bg = { 50, 50, 50, 255 };
    vs_default.filled = true;
    style_register(vs_default, "default");
}

void style_clear(view_style_t& style)
{
    style.font = "";
    style.italic = false;
    style.bold = false;
    style.underline = false;

    style.fg = { 0, 0, 0, 0 };
    style.bg = { 0, 0, 0, 0 };
    style.filled = false;

    style.border_color = { 0, 0, 0, 0 };
    memset(&style.border, 0, sizeof(borders_t));
    memset(&style.border_radius, 0, sizeof(borders_t));
    memset(&style.margin, 0, sizeof(borders_t));
    memset(&style.padding, 0, sizeof(borders_t));
}

void style_register(view_style_t style, std::string name)
{
    styleMap[name] = style;
}

view_style_t style_get(std::string name)
{
    if (styleMap.find(name) == styleMap.end()) {
        name = "default";
    }
    return styleMap[name];
}
