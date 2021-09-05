#include "image.h"
#include "render_cache.h"

icon_view::icon_view()
    : view_item("icon")
    , icon(0)
{
    layout()->width = 32;
    layout()->height = 24;
}

void icon_view::render()
{
    if (icon) {
        draw_image(icon, { 4 + layout()->render_rect.x, layout()->render_rect.y, 24, 24 });
    }
}