#ifndef POPUP_VIEW_H
#define POPUP_VIEW_H

#include "panel.h"
#include "view.h"

#define POPUP_DIRECTION_DOWN 1 << 1
#define POPUP_DIRECTION_UP 1 << 2
#define POPUP_DIRECTION_RIGHT 1 << 3
#define POPUP_DIRECTION_LEFT 1 << 4

struct popup_view : panel_view {
    popup_view();

    int direction;
    view_item* pm;

    layout_rect attach_to;
};

struct popup_manager : view_item {
    popup_manager();

    void push_at(view_item_ptr popup, layout_rect attach = { 0, 0, 0, 0 }, int direction = POPUP_DIRECTION_DOWN);
    void push(view_item_ptr popup);
    void pop();
};

#endif // POPUP_VIEW_H