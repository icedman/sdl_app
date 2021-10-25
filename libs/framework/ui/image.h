#ifndef IMAGE_H
#define IMAGE_H

#include "renderer.h"
#include "view.h"

struct image_view_t : view_t {

    DECLAR_VIEW_TYPE(IMAGE, view_t)

    void render(renderer_t* renderer) override;
    void load_image(std::string path, int w, int h);

    image_ptr image;
};

struct icon_view_t : image_view_t {

    DECLAR_VIEW_TYPE(IMAGE, image_view_t)

    void render(renderer_t* renderer) override;
    void load_icon(std::string path, std::string character);

    std::string character;
    font_ptr icon_font;
};

#endif // IMAGE_H
