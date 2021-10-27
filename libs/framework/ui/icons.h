#ifndef ICONS_H
#define ICONS_H

#include "image.h"
#include "renderer.h"

#include <memory>

struct icon_view_t : image_view_t {
    DECLAR_VIEW_TYPE(ICON, image_view_t)
};

struct icons_factory_t {
    static icons_factory_t* instance();

    void load_icons(std::string path, int w = 24, int h = 24);
    image_ptr icon(std::string name);
};

#endif // ICONS_H
