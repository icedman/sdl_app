#include "text.h"
#include "renderer.h"

#include "../libs/editor/util.h"

text_view::text_view(std::string text)
    : view_item("text")
    , text(text)
{
    layout()->height = 40;
    pad = 2;

    if (Renderer::instance()->is_terminal()) {
        layout()->height = 1;
        pad = 0;
    }

    // cache_enabled = true;
}

void text_view::prelayout()
{
    Renderer::instance()->get_font_extents(Renderer::instance()->font("ui-small"), &text_width, &text_height, text.c_str(), text.length());
    layout()->width = text_width + pad * 2;
    if (Renderer::instance()->is_terminal()) {
        layout()->width = text.length();
    }
}

void text_view::render()
{
    layout_item_ptr lo = layout();

    if (!cache_enabled) {
        Renderer::instance()->draw_text(Renderer::instance()->font("ui-small"), (char*)text.c_str(),
            lo->render_rect.x + pad,
            lo->render_rect.y + pad + ((lo->render_rect.h - (pad * 2)) / 2) - (text_height / 2),
            { 255, 255, 255 },
            false, false);
        return;
    }

    if (text != cached_text) {
        destroy_cache();
    }

    if (cached_image) {
        Renderer::instance()->draw_image(cached_image, { lo->render_rect.x, lo->render_rect.y, lo->width, lo->render_rect.h });
        return;
    }

    cached_image = cache(lo->width, lo->render_rect.h);
    cached_text = text;

    Renderer::instance()->begin_frame(cached_image);
    Renderer::instance()->draw_text(Renderer::instance()->font("ui-small"), (char*)text.c_str(),
        pad,
        pad + ((lo->render_rect.h - (pad * 2)) / 2) - (text_height / 2),
        { 255, 255, 255 },
        false);
    Renderer::instance()->end_frame();

    render();
}