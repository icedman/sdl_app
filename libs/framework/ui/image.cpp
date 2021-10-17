#include "image.h"
#include "damage.h"
#include "renderer.h"
#include "system.h"

image_view_t::image_view_t()
    : view_t()
    , icon(0)
{
}

image_view_t::~image_view_t()
{
    if (icon) {
        renderer_t::destroy_image(icon);
    }
}

void image_view_t::render(renderer_t* renderer)
{
    if (!icon) {
        return;
    }

    layout_item_ptr item = layout();
    system_t::instance()->renderer.draw_image(icon, { item->render_rect.x, item->render_rect.y, icon->width, icon->height });
}

void image_view_t::load_icon(std::string path, int w, int h)
{
    layout()->width = w;
    layout()->height = h;

    if (icon) {
        renderer_t::destroy_image(icon);
    }

    icon = system_t::instance()->renderer.create_image_from_svg(path, w, h);
}
