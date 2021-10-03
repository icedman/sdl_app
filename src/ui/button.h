#ifndef BUTTON_VIEW_H
#define BUTTON_VIEW_H

#include "text.h"
#include "view.h"

struct button_view : view_item {
    button_view(std::string text);

    view_item_ptr text;

    DECLAR_VIEW_TYPE(BUTTON, view_item)
};

#endif // BUTTON_VIEW_H