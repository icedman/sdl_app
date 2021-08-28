#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include "view.h"

struct text_view : view_item {
    text_view(std::string text);

    void precalculate() override;
    
    std::string text;
};

#endif // TEXT_VIEW_H