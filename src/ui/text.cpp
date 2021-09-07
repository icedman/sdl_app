#include "text.h"
#include "renderer.h"
#include "render_cache.h"

text_view::text_view(std::string text)
    : view_item("text")
    , text(text)
{
    layout()->height = 40;
    // cache_enabled = true;
}

void text_view::prelayout()
{
    ren_get_font_extents(ren_font("ui-small"), &text_width, &text_height, text.c_str(), text.length(), false);
    int pad = 2;
    layout()->width = text_width + pad*2;
}

void text_view::render()
{    
    layout_item_ptr lo = layout();
    int pad = 2;

    if (!cache_enabled) {
        draw_text(ren_font("ui-small"), (char*)text.c_str(), 
            lo->render_rect.x + pad,
            lo->render_rect.y + pad + ((lo->render_rect.h - (pad*2))/2) - (text_height/2),
            { 255, 255, 255 },
            false, false, true);
        return;
    }

    if (text != cached_text) {
        destroy_cache();
    }

    if (cached_image) {
        draw_image(cached_image, {
            lo->render_rect.x,
            lo->render_rect.y,
            lo->width,
            lo->render_rect.h
        });
        return;
    }

    cached_image = cache(lo->width, lo->render_rect.h);
    cached_text = text;

    ren_begin_frame(cached_image);
    ren_draw_text(ren_font("ui-small"), (char*)text.c_str(), 
        pad,
        pad + ((lo->render_rect.h - (pad*2))/2) - (text_height/2),
        { 255, 255, 255 },
        false, false, true);
    ren_end_frame();

    render();
}