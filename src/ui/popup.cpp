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

void popup_manager::push_at(view_item_ptr popup, int x, int y, int w, int h, int direction)
{
    push(popup);
    popup->layout()->x = x;
    popup->layout()->y = y;
    view_item::cast<popup_view>(popup)->direction = POPUP_DIRECTION_DOWN;
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
