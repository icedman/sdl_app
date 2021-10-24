#ifndef POPUP_H
#define POPUP_H

#include "panel.h"
#include "view.h"

#define POPUP_DIRECTION_DOWN 1 << 1
#define POPUP_DIRECTION_UP 1 << 2
#define POPUP_DIRECTION_RIGHT 1 << 3
#define POPUP_DIRECTION_LEFT 1 << 4

struct popup_t : view_t {
    popup_t();

    DECLAR_VIEW_TYPE(POPUP, view_t)

    view_ptr content() { return _content; }
    view_ptr _content;

    int direction;
    view_t* pm;
    rect_t attach_to;
};

struct popup_manager_t : view_t {

    static view_ptr instance();

    popup_manager_t();

    void push_at(view_ptr popup, rect_t attach = { 0, 0, 0, 0 }, int direction = POPUP_DIRECTION_DOWN);
    void push(view_ptr popup);
    void pop();
    void clear();

    view_ptr last_focused;
};

#endif // POPUP_H