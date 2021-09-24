#include "style.h"
#include "app.h"

#include <algorithm>
#include <map>

std::map<std::string, view_style_t> styleMap;

void view_style_register(view_style_t style, std::string name)
{
    styleMap[name] = style;
}

view_style_t view_style_get(std::string name)
{
    // app_t::log("style? %s", name.c_str());
    if (styleMap.find(name) == styleMap.end()) {
        name = "default";
    }
    return styleMap[name];
}