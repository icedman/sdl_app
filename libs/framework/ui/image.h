#ifndef IMAGE_H
#define IMAGE_H

#include "renderer.h"
#include "view.h"

struct image_view_t : view_t {

    DECLAR_VIEW_TYPE(IMAGE, view_t)

    void render(renderer_t* renderer) override;
    void set_image(image_ptr img);
    void load_image(std::string path, int w, int h);

    image_ptr image;
};

#endif // IMAGE_H
