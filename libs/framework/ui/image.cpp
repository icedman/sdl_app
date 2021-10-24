#include "image.h"
#include "damage.h"
#include "system.h"

image_view_t::image_view_t()
    : view_t()
{
}

image_view_t::~image_view_t()
{
}

void image_view_t::render(renderer_t* renderer)
{
    if (!icon) {
        return;
    }

    float sz = 0.75f;
    layout_item_ptr item = layout();
    system_t::instance()->renderer.draw_image(icon.get(),
        { item->render_rect.x + icon->width * (1.0f - sz), item->render_rect.y, icon->width * sz, icon->height * sz },
        { 255, 255, 255, 0 });
}

void image_view_t::load_icon(std::string path, int w, int h)
{
    layout()->width = w;
    layout()->height = h;
    icon = system_t::instance()->renderer.create_image_from_svg(path, w, h);
}
