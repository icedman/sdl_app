#include "text.h"
#include "renderer.h"
#include "render_cache.h"

text_view::text_view(std::string text)
    : view_item("text")
    , text(text)
{
    layout()->height = 40;
}

void text_view::prelayout()
{
    ren_get_font_extents(NULL, &text_width, &text_height, text.c_str(), text.length(), false);

    int pad = 2;
    layout()->width = text_width + pad*2;
}

void text_view::render()
{
    int pad = 2;
    layout_item_ptr lo = layout();
    draw_text(NULL, (char*)text.c_str(), 
        lo->render_rect.x + pad,
        lo->render_rect.y + pad + ((lo->render_rect.h - (pad*2))/2) - (text_height/2),
        { 255, 255, 255 },
        false, false, true);
}