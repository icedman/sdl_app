#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "view.h"

struct text_view : view_item {
    text_view(std::string text);

    void prelayout() override;
    void render() override;
    
    std::string text;
    int text_width;
    int text_height;
};

#endif // TEXT_VIEW_H