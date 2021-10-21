#ifndef TEXT_BLOCK_H
#define TEXT_BLOCK_H

#include "renderer.h"
#include "view.h"

struct layout_text_span_t : layout_item_t {
    int start;
    int length;
    std::string text;
};

struct text_block_t : horizontal_container_t {
    text_block_t(std::string = "");

    DECLAR_VIEW_TYPE(view_type_e::TEXT, horizontal_container_t)
    std::string type_name() override { return "text_block"; }

    void set_text(std::string text);
    std::string text();

    std::string _text;
    std::string _prev_text;
    std::vector<text_span_t> _text_spans;
    bool wrapped;

    void prelayout() override;
    void render(renderer_t* renderer) override;

    int content_hash(bool peek = false) override;
};

#endif TEXT_BLOCK_H