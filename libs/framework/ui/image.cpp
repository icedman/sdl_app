#include "image.h"
#include "damage.h"
#include "system.h"
#include "utf8.h"

void image_view_t::render(renderer_t* renderer)
{
    if (!image) {
        return;
    }

    float sz = 1.0f;
    layout_item_ptr item = layout();
    rect_t rect = item->render_rect;
    rect.w *= sz;
    rect.h *= sz;

    if (image->width < image->height) {
        rect.h = rect.w * (image->width / image->height);
    } else {
        rect.w = rect.h * (image->height / image->width);
    }

    system_t::instance()->renderer.draw_image(image.get(),
        rect,
        system_t::instance()->renderer.foreground);
}

void image_view_t::set_image(image_ptr img)
{
    image = img;
}

void image_view_t::load_image(std::string path, int w, int h)
{
    layout()->width = w;
    layout()->height = h;
    image = system_t::instance()->renderer.create_image_from_svg(path, w, h);
}
