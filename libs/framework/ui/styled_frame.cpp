#include "view.h"
#include "styled_frame.h"
#include "renderer.h"

void render_styled_frame(renderer_t* renderer, rect_t rect, styled_frame_t& style)
{
    rect_t r = rect;
    color_t default_clr = { 150,150,150, 255 };

    // margin
    r.x += style.margin.width;
    r.y += style.margin.width;
    r.w -= (style.margin.width + style.margin.width);
    r.h -= (style.margin.width + style.margin.width);

    if (style.filled) {
        color_t clr = default_clr;
        if (color_is_set(style.bg)) {
            clr = style.bg;
        }
        renderer->draw_rect(r, clr, true, 0, {}, style.border_radius.width);
    }

    if (style.border.width) {
        color_t clr = default_clr;
        if (color_is_set(style.border_color)) {
            clr = style.border_color;
        }
        renderer->draw_rect(r, clr, false, style.border.width, clr, style.border_radius.width);
    }
}
