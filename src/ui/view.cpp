#include "view.h"
#include "popup.h"
#include "render_cache.h"

static view_item* view_root = 0;
static view_item* view_focused = 0;
static view_item* view_hovered = 0;
static view_item* view_pressed = 0;
static view_item* view_released = 0;
static view_item* view_clicked = 0;
static view_item* view_dragged = 0;
static int drag_start_x = 0;
static int drag_start_y = 0;
static bool dragging = false;
static int _keyMods = 0;

view_item_list popups;
void view_input_list(view_item_list& list, view_item_ptr item)
{
    if (!item->_layout || item->disabled || !item->layout()->visible || item->layout()->offscreen) {
        return;
    }

    if (!list.size()) {
        popups.clear();
    } else {
        if (item->type == "popup") {
            popups.push_back(item);
        }
    }

    if (item->interactive) {
        list.insert(list.begin(), 1, item);
    }
    
    for (auto child : item->_views) {
        view_input_list(list, child);
    }
}

view_item::view_item()
    : view_item("view")
{
}

view_item::view_item(std::string type)
    : type(type)
    , cached_image(0)
    , cache_enabled(false)
{
}

view_item::~view_item()
{
    destroy_cache();

    if (view_focused == this)
        view_focused = 0;
    if (view_hovered == this)
        view_hovered = 0;
    if (view_pressed == this)
        view_pressed = 0;
    if (view_released == this)
        view_released = 0;
    if (view_clicked == this)
        view_clicked = 0;
    if (view_dragged == this)
        view_dragged = 0;
}

RenImage* view_item::cache(int w, int h)
{
    if (cached_image) {
        int cw, ch;
        ren_image_size(cached_image, &cw, &ch);
        if (cw != w || ch != h) {
            destroy_cache();
        }
    }
    if (!cached_image) {
        cached_image = ren_create_image(w, h);
    }
    return cached_image;
}

void view_item::destroy_cache()
{
    if (cached_image) {
        ren_destroy_image(cached_image);
        cached_image = 0;
    }
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
    for (auto i : layout()->children) {
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
    while (it != _views.end()) {
        view_item_ptr v = *it;
        if (v == view) {
            _views.erase(it);
            break;
        }
        it++;
    }

    layout_item_list::iterator lit = layout()->children.begin();
    while (lit != layout()->children.end()) {
        layout_item_ptr l = *lit;
        if (l == view->layout()) {
            layout()->children.erase(lit);
            return;
        }
        lit++;
    }
}

void view_item::relayout()
{
    if (parent) {
        view_item* v = (view_item*)parent;
        layout_run(v->layout(), v->layout()->constraint);
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
    return this == view_dragged;
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
    for (auto v : _views) {
        v->update();
    }
}

void view_item::render()
{
    #if 0
    layout_item_ptr lo = layout();

    if (!lo->visible) return;
    
    draw_rect({ lo->render_rect.x,
                  lo->render_rect.y,
                  lo->render_rect.w,
                  lo->render_rect.h },
        { color.r, color.g, color.b, color.a }, false, 1, 0);
    #endif
}

int view_item::on(event_type_e event_type, event_callback_t callback)
{
    callbacks[event_type].insert(callbacks[event_type].begin(), callback);
    return 0;
}

void view_item::propagate_event(event_t& event)
{
    if (parent) {
        ((view_item*)parent)->propagate_event(event);
        // if (event.cancelled) return;
    }
    for (auto c : callbacks[event.type]) {
        c(event);
        if (event.cancelled)
            break;
    }
}

view_item_ptr view_find_xy(view_item_ptr item, int x, int y)
{
    if (!item->layout()->visible || item->layout()->offscreen) {
        return 0;
    }

    layout_rect r = item->layout()->render_rect;
    if ((x > r.x && x < r.x + r.w) && (y > r.y && y < r.y + r.h)) {
        // within
    } else {
        return 0;
    }

    for (auto v : item->_views) {
        // check child
        view_item_ptr vc = view_find_xy(v, x, y);
        if (vc) {
            return vc;
        }
    }

    if (!item->disabled && item->interactive) {
        // printf(">%s\n", item->type.c_str());
        return item;
    }

    return 0;
}

view_item_list* _view_list;
void view_input_events(view_item_list& list, event_list& events)
{
    view_clicked = 0;

    _view_list = &list;
    for (auto e : events) {

        e.source = 0;
        e.target = 0;
        e.cancelled = false;

        switch (e.type) {
        case EVT_KEY_UP:
            _keyMods = e.mod;
            break;
        case EVT_KEY_DOWN:
            _keyMods = e.mod;
            view_input_key(e.key, e);
            break;
        case EVT_KEY_TEXT:
            view_input_text(e.text, e);
            break;
        case EVT_KEY_SEQUENCE:
            view_input_sequence(e.text, e);
            break;
        case EVT_MOUSE_WHEEL:
            view_input_wheel(e.x, e.y, e);
            break;
        case EVT_MOUSE_DOWN:
            view_input_button(e.button, e.x, e.y, 1, e.clicks, e);
            break;
        case EVT_MOUSE_UP:
            view_input_button(e.button, e.x, e.y, 0, 0, e);
            break;
        case EVT_MOUSE_MOTION:
            view_input_button(e.button, e.x, e.y, e.button != 0, 0, e);
            break;
        }
    }
}

void view_input_button(int button, int x, int y, int pressed, int clicks, event_t event)
{
    view_item* v = 0;
    view_item_ptr _v;

    if (popups.size()) {
        _v = view_find_xy(popups.back(), x, y);
        if (!_v && pressed && !dragging) {
            popup_view* pop = view_item::cast<popup_view>(popups.back());
            popup_manager* pm = (popup_manager*)(pop->pm);
            pm->pop();
            if (!popups.size()) {
                _v = view_find_xy((*_view_list).back(), x, y);
            }
        }
    } else {
        _v = view_find_xy((*_view_list).back(), x, y);
    }

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
    if (view_hovered) {
        event.type = EVT_MOUSE_MOTION;
        event.source = view_hovered;
        view_hovered->propagate_event(event);
    }

    if (view_pressed) {
        if (!dragging) {
            int dx = drag_start_x - x;
            int dy = drag_start_y - y;
            int drag_distance = dx * dx + dy * dy;
            if (drag_distance >= 4) {
                event.type = EVT_MOUSE_DRAG_START;
                event.source = view_pressed;
                view_pressed->propagate_event(event);
                view_dragged = view_pressed;
                dragging = true;
            }
        } else {
            event.type = EVT_MOUSE_DRAG;
            event.source = view_pressed;
            view_pressed->propagate_event(event);
        }
    }

    if (pressed) {
        if (!dragging) {
            drag_start_x = x;
            drag_start_y = y;
        }
        if (!view_pressed) {
            view_pressed = v;
            if (view_pressed) {
                event.type = EVT_MOUSE_DOWN;
                event.source = view_pressed;
                view_pressed->propagate_event(event);
            }
        }
        if (v && v->focusable) {
            view_focused = v;
        }
    } else {
        if (dragging) {
            if (view_pressed) {
                event.type = EVT_MOUSE_DRAG_END;
                event.source = view_pressed;
                view_pressed->propagate_event(event);
            }
        }
        view_released = v ? v : 0;
        view_clicked = view_released == view_pressed ? view_released : 0;
        if (view_clicked && !dragging) {
            event.type = EVT_MOUSE_CLICK;
            event.source = view_clicked;
            view_clicked->propagate_event(event);
        }
        view_pressed = 0;
        if (view_released) {
            event.type = EVT_MOUSE_UP;
            event.source = view_released;
            view_released->propagate_event(event);
        }
    }

    if (view_pressed) {
        view_hovered = view_pressed;
    }

    if (!v || !pressed) {
        dragging = false;
    }
}

void view_input_wheel(int x, int y, event_t event)
{
    if (popups.size()) {
        view_item_ptr _v = popups.back();
        _v = view_find_xy(_v, 4, 4);
        if (_v) {
            view_hovered = _v.get();
        }
    }
    if (view_hovered) {
        printf("%s\n", view_hovered->type.c_str());
        event.type = EVT_MOUSE_WHEEL;
        event.source = view_hovered;
        view_hovered->propagate_event(event);
    }
}

void view_input_key(int key, event_t event)
{
    if (view_focused) {
        event.type = EVT_KEY_DOWN;
        event.source = view_focused;
        view_focused->propagate_event(event);
    }
}

void view_input_text(std::string text, event_t event)
{
    if ((view_input_key_mods() & K_MOD_CTRL) == K_MOD_CTRL || (view_input_key_mods() & K_MOD_ALT) == K_MOD_ALT) {
        return;
    }
    if (view_focused) {
        event.type = EVT_KEY_TEXT;
        event.source = view_focused;
        view_focused->propagate_event(event);
    }
}

void view_input_sequence(std::string sequence, event_t event)
{
    if (view_root) {
        event.type = EVT_KEY_SEQUENCE;
        event.source = view_focused;
        view_root->propagate_event(event);
        if (event.cancelled) {
            return;
        }
    }
    if (view_focused) {
        event.type = EVT_KEY_SEQUENCE;
        event.source = view_focused;
        view_focused->propagate_event(event);
    }
}

int view_input_key_mods()
{
    return _keyMods;
}

void view_set_focused(view_item* item)
{
    view_focused = item;
}

view_item* view_get_focused()
{
    return view_focused;
}

view_item* view_get_root()
{
    return view_root;
}

void view_set_root(view_item* item)
{
    view_root = item;
}
