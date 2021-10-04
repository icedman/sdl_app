#include "popup.h"
#include "renderer.h"

popup_view::popup_view()
    : panel_view()
    , direction(POPUP_DIRECTION_DOWN)
{
    layout()->stack = true;
}

popup_manager::popup_manager()
    : view_item()
{
    layout()->stack = true;
}

void popup_manager::push_at(view_item_ptr popup, layout_rect attach, int direction)
{
    push(popup);

    layout_item_ptr lo = popup->layout();
    ((view_item*)lo->view)->prev_visibility = false;

    lo->x = attach.x;
    lo->y = attach.y;
    if (direction & POPUP_DIRECTION_DOWN) {
        lo->y += attach.h;
    }
    if (direction & POPUP_DIRECTION_RIGHT) {
        lo->x += attach.w;
    }
    if (direction & POPUP_DIRECTION_UP) {
        lo->y -= lo->height;
    }
    if (direction & POPUP_DIRECTION_LEFT) {
        lo->x -= lo->width;
    }
    view_item::cast<popup_view>(popup)->direction = direction;
    view_item::cast<popup_view>(popup)->attach_to = attach;
}

void popup_manager::push(view_item_ptr popup)
{
    view_item::cast<popup_view>(popup)->pm = this;
    popup->layout()->stack = true;

    add_child(popup);
    layout_request();
}

void popup_manager::pop()
{
    if (_views.size()) {
        remove_child(_views.back());
        damage();
    }
}

void popup_manager::clear()
{
    while (_views.size()) {
        pop();
    }
}