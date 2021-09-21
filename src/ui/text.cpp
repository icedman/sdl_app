#include "text.h"
#include "renderer.h"
#include "style.h"

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
    view_style_t vs = view_style_get("explorer");

    if (!cache_enabled) {
        Renderer::instance()->draw_text(Renderer::instance()->font("ui-small"), (char*)text.c_str(),
            lo->render_rect.x + pad,
            lo->render_rect.y + pad + ((lo->render_rect.h - (pad * 2)) / 2) - (text_height / 2),
            { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue, vs.fg.index },
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
        { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue },
        false);
    Renderer::instance()->end_frame();

    render();
}