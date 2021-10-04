#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "view.h"

struct text_view : view_item {
    text_view(std::string text);

    DECLAR_VIEW_TYPE(TEXT, view_item)

    void prelayout() override;

    void prerender() override;
    void render() override;

    std::string text;
    std::string cached_text;
    int text_width;
    int text_height;
    int pad;

    int prev_width;
    int prev_height;
    std::string prev_text;
};

#endif // TEXT_VIEW_H