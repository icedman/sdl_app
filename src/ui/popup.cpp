#include "popup.h"
#include "renderer.h"

popup_view::popup_view()
    : panel_view()
    , direction(POPUP_DIRECTION_DOWN)
{
    type = "popup";
    layout()->stack = true;
}

popup_manager::popup_manager()
    : view_item("popup_manager")
{
    layout()->stack = true;
}

void popup_manager::push_at(view_item_ptr popup, layout_rect attach, int direction)
{
    push(popup);

    layout_item_ptr lo = popup->layout();
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
    popup->type = "popup";
    add_child(popup);
    layout_request();
}

void popup_manager::pop()
{
    remove_child(_views.back());
}
