#include "popup.h"
#include "events.h"
#include "renderer.h"

static view_ptr global_popups;

view_ptr popup_manager_t::instance()
{
    if (!global_popups) {
        global_popups = std::make_shared<popup_manager_t>();
    }
    return global_popups;
}

popup_t::popup_t()
    : view_t()
    , direction(POPUP_DIRECTION_DOWN)
{
    layout()->stack = true;

    _content = std::make_shared<vertical_container_t>();
    _content->layout()->fit_children_x = true;
    _content->layout()->fit_children_y = true;

    add_child(_content);
}

popup_manager_t::popup_manager_t()
    : view_t()
{
    layout()->stack = true;

    events_manager_t::instance()->on(EVT_ALL, [this](event_t& event) {
        if (!this->children.size())
            return false;

        switch (event.type) {
        case EVT_WINDOW_RESIZE:
            this->clear();
            return false;
        case EVT_KEY_SEQUENCE:
            if (event.text == "escape") {
                this->clear();
            }
            return false;
        default:
            return false;
        }

        return false;
    });
}

void popup_manager_t::push_at(view_ptr popup, rect_t attach, int direction)
{
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

    popup->cast<popup_t>()->direction = direction;
    popup->cast<popup_t>()->attach_to = attach;

    push(popup);
}

void popup_manager_t::push(view_ptr popup)
{
    if (!parent)
        return;

    popup->cast<popup_t>()->pm = this;
    popup->layout()->stack = true;

    layout_clear_hash(popup->layout(), 8);

    view_list::iterator it = std::find(children.begin(), children.end(), popup);
    if (it != children.end()) {
        remove_child(popup);
        return;
    }

    add_child(popup);
    relayout();

    // push focused
    if (view_current_focused() && !view_current_focused()->is_type_of(view_type_e::POPUP)) {
        last_focused = view_current_focused();
    }

    set_focused(popup.get());
}

void popup_manager_t::pop()
{
    if (children.size()) {
        remove_child(children.back());
    }

    if (!children.size()) {
        set_focused(last_focused.get());
    }
}

void popup_manager_t::clear()
{
    while (children.size()) {
        pop();
    }
}
