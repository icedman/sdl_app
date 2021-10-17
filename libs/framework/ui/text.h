#ifndef TEXT_H
#define TEXT_H

#include "renderer.h"
#include "view.h"

struct text_t : view_t {
    text_t(std::string = "");

    DECLAR_VIEW_TYPE(view_type_e::TEXT, view_t)

    void set_text(std::string text);
    std::string text();

    std::string _text;
    std::vector<text_span_t> _text_spans;

    void prelayout() override;
    void render(renderer_t* renderer) override;

    int content_hash(bool peek = false) override;
};

#endif TEXT_H