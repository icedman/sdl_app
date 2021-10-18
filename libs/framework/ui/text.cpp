#include "text.h"
#include "damage.h"
#include "hash.h"
#include "renderer.h"
#include "system.h"

text_t::text_t(std::string t)
    : view_t()
{
    _text = t;
    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };
}

std::string text_t::text()
{
    return _text;
}

void text_t::set_text(std::string text)
{
    _text = text;
    layout()->content_hash = murmur_hash(text.c_str(), text.length(), CONTENT_HASH_SEED);

    // layout_item_ptr item = layout();
    // prelayout();
    // if ((item->width && item->render_rect.w != item->width) || (item->height && item->render_rect.h != item->height)) {
    //     relayout();
    // }
}

void text_t::prelayout()
{
    layout_item_ptr item = layout();
    item->height = font()->height;
    item->width = font()->width * _text.length();
    if (!item->width) {
        item->width = 1;
    }
}

void text_t::render(renderer_t* renderer)
{
    layout_item_ptr item = layout();
    renderer->begin_text_span(_text_spans);
    renderer->draw_text(NULL, (char*)_text.c_str(), item->render_rect.x, item->render_rect.y, { 255, 255, 255 });
    renderer->end_text_span();
}

int text_t::content_hash(bool peek)
{
    int hash;
    if (_text.length()) {
        hash = murmur_hash(_text.c_str(), _text.length(), CONTENT_HASH_SEED);
        if (!peek) {
            _content_hash = hash;
        }
    }
    return hash;
}