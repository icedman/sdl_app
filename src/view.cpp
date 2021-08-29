#include "view.h"

static view_item *view_focused = 0;
static view_item *view_hovered = 0;
static view_item *view_pressed = 0;
static view_item *view_released = 0;
static view_item *view_clicked = 0;
static view_item *view_dragged = 0;
static int drag_start_x = 0;
static int drag_start_y = 0;

void view_input_list(view_item_list &list, view_item_ptr item)
{
    if (!item->_layout || !item->layout()->visible) {
        return;
    }
    if (item->disabled) {
        return;
    }
    list.insert(list.begin(), 1, item);
    for(auto child : item->_views) {
        view_input_list(list, child);
    }
}

view_item::view_item()
    : type("view")
{}

view_item::view_item(std::string type)
    : type(type)
{}

layout_item_ptr view_item::layout()
{
    if (!_layout) {
        _layout = std::make_shared<layout_item>();
        _layout->view = this;
        _layout->name = type;
    }
    return _layout;
}

void view_item::add_child(view_item_ptr view)
{
    for(auto i : layout()->children) {
        if (i == view->layout()) {
            return;
        }
    }
    _views.push_back(view);
    layout()->children.push_back(view->layout());
}

void view_item::remove_child(view_item_ptr view)
{
    view_item_list::iterator it = _views.begin();
    while(it++ != _views.end()) {
        if (*it == view) {
            _views.erase(it);
            break;
        }
    }

    layout_item_list::iterator lit = layout()->children.begin();
    while(lit++ != layout()->children.end()) {
        if (*lit == view->layout()) {
            layout()->children.erase(lit);
            return;
        }
    }
}

bool view_item::is_focused()
{
    return this == view_focused;
}

bool view_item::is_pressed()
{
    return this == view_pressed;
}

bool view_item::is_dragged()
{
    return false;
}

bool view_item::is_hovered()
{
    return this == view_hovered;
}

bool view_item::is_clicked()
{
    return this == view_clicked;
}

view_item_ptr view_find_xy(view_item_list &list, int x, int y)
{
    for(auto v : list) {
        if (!v->can_press && !v->can_focus && !v->can_hover) {
            continue;
        }
        layout_rect r = v->layout()->render_rect;
        if ((x > r.x && x < r.x + r.w) &&
            (y > r.y && y < r.y + r.h)) {
            // printf("down %d %d\n", r.x , r.y);
            return v;
        }

    }
    return 0;
}

view_item_list *_view_list;
void view_input_events(view_item_list &list, event_list &events)
{
    view_clicked = 0;

    _view_list = &list;
    for(auto e : events) {
        switch(e.type) {
        case EVT_MOUSE_DOWN:
            view_input_button(e.button, e.x, e.y, true);
            break;
        case EVT_MOUSE_UP:
            view_input_button(e.button, e.x, e.y, false);
            break;
        case EVT_MOUSE_MOTION:
            view_input_button(e.button, e.x, e.y, e.button != 0);
            break;
        }
    }
}

void view_input_button(int button, int x, int y, int pressed)
{
    view_item_ptr v = view_find_xy(*_view_list, x, y);
    view_hovered = 0;
    if (!v) {
        if (!pressed) {
            view_pressed = 0;
            view_released = 0;
        }
        return;
    }

    view_hovered = v->can_hover ? v.get() : 0;
    if (view_hovered) view_hovered->mouse_move(x, y, button);
    if (pressed) {
        if (!view_pressed) {
            view_pressed = v->can_press ? v.get() : 0;
            if (view_pressed) view_pressed->mouse_down(x, y, button);
        }
        if (v->can_focus) {
            view_focused = v.get();
        }
    } else {
        view_released = v->can_press ? v.get() : 0;
        view_clicked = view_released == view_pressed ? view_released : 0;
        view_pressed = 0;
        if (view_released) view_released->mouse_up(x, y, button);
    }

    if (view_pressed) {
        view_hovered = view_pressed->can_hover ? view_pressed : view_hovered;
    }
}
