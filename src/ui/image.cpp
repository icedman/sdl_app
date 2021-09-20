#include "image.h"
#include "renderer.h"

icon_view::icon_view()
    : view_item("icon")
    , icon(0)
{
    layout()->width = 32;
    layout()->height = 24;

    if (Renderer::instance()->is_terminal()) {
        layout()->width = 1;
        layout()->height = 1;
    }
}

void icon_view::render()
{
    if (icon) {
        Renderer::instance()->draw_image(icon, { 8 + layout()->render_rect.x, 4 + layout()->render_rect.y, 18, 18 });
    }
}
