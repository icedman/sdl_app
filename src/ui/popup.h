#ifndef POPUP_VIEW_H
#define POPUP_VIEW_H

#include "view.h"
#include "panel.h"


#define POPUP_DIRECTION_DOWN    1<<1
#define POPUP_DIRECTION_UP      1<<2
#define POPUP_DIRECTION_RIGHT   1<<3
#define POPUP_DIRECTION_LEF     1<<4

struct popup_view : panel_view {
    popup_view();

    int direction;
    view_item *pm;
};

struct popup_manager : view_item {
    popup_manager();

    void push_at(view_item_ptr popup, int x, int y, int w = 0, int h = 0, int direction = 0);
    void push(view_item_ptr popup);
    void pop();
};

#endif // POPUP_VIEW_H