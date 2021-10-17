#ifndef IMAGE_H
#define IMAGE_H

#include "view.h"

struct image_t;
struct image_view_t : view_t {
    image_view_t();
    ~image_view_t();

    DECLAR_VIEW_TYPE(IMAGE, view_t)

    void render(renderer_t* renderer) override;
    void load_icon(std::string path, int w, int h);

    image_t* icon;
};

#endif // IMAGE_H
