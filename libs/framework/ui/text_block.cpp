#include "text_block.h"
#include "damage.h"
#include "hash.h"
#include "renderer.h"
#include "system.h"

#include <cstring>
#include <set>

static std::vector<std::string> split_string(const std::string& str)
{
    static std::set<char> delimiters = {
        '.', ',', ';', ':',
        '-', '+', '*', '/', '%', '=',
        '"', ' ', '\'', '\\',
        '(', ')', '[', ']', '<', '>',
        '&', '!', '?', '_', '~', '@'
    };
    // static const char delimiters[] = {
    //     '.', ',', ';', ':',
    //     '-', '+', '*', '/', '%', '=',
    //     '"', ' ', '\'', '\\',
    //     '(', ')', '[', ']', '<', '>',
    //     '&', '!', '?', '_', '~', '@',
    // };

    std::vector<std::string> result;

    char const* line = str.c_str();
    char const* pch = str.c_str();
    char const* start = pch;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            // if (strchr((char*)delimiters, (int)*pch) != NULL) {
            if (pch != start) {
                if (pch - start)
                    result.push_back(std::string(start, pch - start));
            }
            result.push_back(std::string(pch, 1));
            start = pch;
            start++;
        }
    }

    if (pch - start) {
        std::string s = std::string(start, pch - start);
        result.push_back(s);
    }

    return result;
}

text_block_t::text_block_t(std::string t)
    : horizontal_container_t()
    , wrapped(false)
{
    layout()->align = LAYOUT_ALIGN_FLEX_START;
    // layout()->align = LAYOUT_ALIGN_FLEX_END;
    // layout()->align = LAYOUT_ALIGN_CENTER;
    // layout()->align = LAYOUT_ALIGN_STRETCH;
    // layout()->justify = LAYOUT_JUSTIFY_CENTER;
    // layout()->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;

    layout()->wrap = true;
    layout()->fit_children_x = false;
    layout()->fit_children_y = true;

    _text = t;
    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };
}

std::string text_block_t::text()
{
    return _text;
}

void text_block_t::set_text(std::string text)
{
    _text = text;
}

void text_block_t::prelayout()
{
    layout_item_ptr lo = layout();
    lo->height = font()->height;
    lo->width = font()->width * _text.length();
    if (!lo->width) {
        lo->width = 1;
    }

    bool _wrap = wrapped;
    if (parent && parent->layout()->render_rect.w > font()->width * _text.length()) {
        _wrap = false;
    }

    if (_prev_text == _text && lo->children.size() > 0 && _wrap == wrapped) {
        return;
    }

    std::vector<std::string> words;
    if (_wrap) {
        words = split_string(_text);
    } else {
        words.push_back(_text);
    }

    // printf("***************\n<<%s>>\n", _text.c_str());
    // for(auto w : words) {
    //     printf("[%s]\n", w.c_str());
    // }
    // printf("---------------\n");

    while (lo->children.size() < words.size()) {
        lo->children.push_back(std::make_shared<layout_text_span_t>());
    }

    for (auto c : lo->children) {
        c->visible = false;
        c->width = 0;
        c->height = 0;
    }

    layout_item_list::iterator it = lo->children.begin();
    int idx = 0;
    for (auto w : words) {
        layout_item_ptr span = *it++;
        span->visible = true;
        layout_text_span_t* text_span = (layout_text_span_t*)span.get();
        text_span->start = idx;
        text_span->length = w.length();
        text_span->text = w;
        idx += text_span->length;
        span->width = w.length() * font()->width;
        span->height = font()->height;
    }

    _prev_text = _text;
}

void text_block_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();

    renderer->begin_text_span(_text_spans);
    for (auto span : lo->children) {
        layout_text_span_t* text_span = (layout_text_span_t*)span.get();
        if (!text_span->visible)
            continue;

        rect_t r = span->render_rect;

        // renderer->draw_rect(r, {255,0,255}, false, 1);
        renderer->draw_text(font().get(), (char*)text_span->text.c_str(),
            r.x,
            r.y, {});
    }
    renderer->end_text_span();
}

int text_block_t::content_hash(bool peek)
{
    int hash = 0;
    if (_text_spans.size()) {
        hash = murmur_hash(&_text_spans[0], sizeof(text_span_t) * _text_spans.size(), CONTENT_HASH_SEED);
        if (!peek) {
            _content_hash = hash;
        }
    }
    if (hash == 0 && _text.length()) {
        hash = murmur_hash(_text.c_str(), _text.length(), CONTENT_HASH_SEED);
        if (!peek) {
            _content_hash = hash;
        }
    }
    return hash;
}