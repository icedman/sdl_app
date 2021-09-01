#include "view.h"

static view_item *view_focused = 0;
static view_item *view_hovered = 0;
static view_item *view_pressed = 0;
static view_item *view_released = 0;
static view_item *view_clicked = 0;
static view_item *view_dragged = 0;
static int drag_start_x = 0;
static int drag_start_y = 0;
static bool dragging = false;
static int _keyMods = 0;

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
    , _cache(0)
{}

view_item::view_item(std::string type)
    : type(type)
    , _cache(0)
{}

view_item::~view_item()
{
    if (_cache) {
        ren_destroy_image(_cache);
    }
}

RenImage* view_item::cache(int w, int h)
{
    if (_cache) {
        int cw, ch;
        ren_image_size(_cache, &cw, &ch);
        if (cw != w || ch != h) {
            ren_destroy_image(_cache);
            _cache = 0;
        }
    }
    if (!_cache) {
        _cache = ren_create_image(w, h);
    }
    return _cache;
}

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
    view->parent = this;
    _views.push_back(view);
    layout()->children.push_back(view->layout());
}

void view_item::remove_child(view_item_ptr view)
{
    view->parent = 0;    
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

void view_item::update()
{
    for(auto v : _views) {
        v->update();
    }
}

bool view_item::mouse_down(int x, int y, int button, int clicks)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_down(x, y, button, clicks)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_up(int x, int y, int button)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_up(x, y, button)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_click(int x, int y, int button)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_click(x, y, button)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_move(int x, int y, int button)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_move(x, y, button)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_drag_start(int x, int y)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_drag_start(x, y)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_drag_end(int x, int y)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_drag_end(x, y)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_drag(int x, int y)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_drag(x, y)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::mouse_wheel(int x, int y)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->mouse_wheel(x, y)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::on_scroll()
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->on_scroll()) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::input_key(int k)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->input_key(k)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::input_text(std::string text)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->input_text(text)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

bool view_item::input_sequence(std::string text)
{
    view_item *p = (view_item*)parent;
    while(p) {
        if (p->input_sequence(text)) {
            return true;
        }
        p = (view_item*)p->parent;
    }
    return false;
}

view_item_ptr view_find_xy(view_item_ptr item, int x, int y)
{
    layout_rect r = item->layout()->render_rect;
    if ((x > r.x && x < r.x + r.w) &&
        (y > r.y && y < r.y + r.h)) {
        if (!item->layout()->visible) {
            return 0;
        }
    } else {
        return 0;
    }

    for(auto v : item->_views) {
        // check child
        view_item_ptr vc = view_find_xy(v, x, y);
        if (vc) {
            return vc;
        }
    }

    if (!item->disabled) {
        return item;
    }
    return 0;

    // return item;
}

view_item_list *_view_list;
void view_input_events(view_item_list &list, event_list &events)
{
    view_clicked = 0;

    _view_list = &list;
    for(auto e : events) {
        switch(e.type) {
        case EVT_KEY_UP:
            _keyMods = e.mod;
            break;
        case EVT_KEY_DOWN:
            _keyMods = e.mod;
            view_input_key(e.key);
            break;
        case EVT_KEY_TEXT:
            view_input_text(e.text);
            break;
        case EVT_KEY_SEQUENCE:
            view_input_sequence(e.text);
            break;
        case EVT_MOUSE_WHEEL:
            view_input_wheel(e.x, e.y);
            break;
        case EVT_MOUSE_DOWN:
            view_input_button(e.button, e.x, e.y, 1, e.clicks);
            break;
        case EVT_MOUSE_UP:
            view_input_button(e.button, e.x, e.y, 0);
            break;
        case EVT_MOUSE_MOTION:
            view_input_button(e.button, e.x, e.y, e.button != 0);
            break;
        }
    }
}

void view_input_button(int button, int x, int y, int pressed, int clicks)
{
    view_item *v = 0;
    view_item_ptr _v = view_find_xy((*_view_list).back(), x, y);
    if (_v) {
        v = _v.get();
    }

    view_hovered = 0;
    if (!v) {
        if (!pressed) {
            view_pressed = 0;
            view_released = 0;
        }
    }
    if (!v) {
        v = view_pressed;
    }

    // printf("%s\n", v->type.c_str());

    view_hovered = v;
    if (view_hovered) view_hovered->mouse_move(x, y, button);

    if (view_pressed) {
        if (!dragging) {
            printf(">start %s\n", v->type.c_str());
            int dx = drag_start_x - x;
            int dy = drag_start_y - y;
            int drag_distance = dx * dx + dy *dy;
            if (drag_distance >= 4) {
                    view_pressed->mouse_drag_start(drag_start_x, drag_start_y);
                    dragging = true; 
            }
        } else {
            printf(">drag %s\n", v->type.c_str());
            view_pressed->mouse_drag(x, y);
        }
    }

    if (pressed) {
        if (!dragging) {
            drag_start_x = x;
            drag_start_y = y;
        }
        if (!view_pressed) {
            view_pressed = v;
            if (view_pressed) view_pressed->mouse_down(x, y, button, clicks);
        }
        if (v && v->can_focus) {
            view_focused = v;
        }
    } else {
        if (dragging) {
            if (view_pressed) {
                view_pressed->mouse_drag_end(x, y);
            }
        }
        view_released = v ? v : 0;
        view_clicked = view_released == view_pressed ? view_released : 0;
        if (view_clicked && !dragging) {
            view_clicked->mouse_click(x, y, button);
        }
        view_pressed = 0;
        if (view_released) view_released->mouse_up(x, y, button);
    }

    if (view_pressed) {
        view_hovered = view_pressed;
    }

    if (!v || !pressed) {
        dragging = false;
    }
}

void view_input_wheel(int x, int y)
{
    if (view_hovered) {
        view_hovered->mouse_wheel(x, y);
    }
}

void view_input_key(int key)
{
    if (view_focused) {
        view_focused->input_key(key);
    }
}

void view_input_text(std::string text)
{
    if (view_focused) {
        view_focused->input_text(text);
    }
}

void view_input_sequence(std::string sequence)
{
    if (view_focused) {
        view_focused->input_sequence(sequence);
    }}

int view_input_key_mods()
{
    return _keyMods;
}

void view_set_focused(view_item *item)
{
    view_focused = item;
}