#include "view.h"
#include "damage.h"
#include "hash.h"
#include "popup.h"
#include "renderer.h"
#include "system.h"

#include "text.h"

#define DRAG_THRESHOLD 25 // distance squared

view_ptr view_hovered;
view_ptr view_focused;
view_ptr view_pressed;
view_ptr view_released;
view_ptr view_dragged;
bool dragging = false;
int drag_start_x;
int drag_start_y;
int mouse_x = -1;
int mouse_y = -1;

view_list entering_views;
view_list exiting_views;

const char* view_type_names[] = {
    "container",
    "spacer",
    "text",
    "text_block",
    "button",
    "image",
    "panel",
    "popup",
    "list",
    "list_item",
    "inputtext",
    "scrollarea",
    "scrollbar",
    "tabbar",
    "tabbed_content",
    "splitter",
    "custom"
};

view_t::view_t()
    : parent(0)
    , disabled(false)
    , can_focus(false)
    , can_hover(false)
    , _font(0)
    , _state_hash(0)
    , _content_hash(0)
    , render_priority(0)
{
    state.style.available = false;
}

view_t::~view_t()
{
}

std::string view_t::type_name()
{
    if (type_of() <= view_type_e::CUSTOM) {
        return view_type_names[type_of()];
    }
    return "";
}

void view_t::add_child(view_ptr view)
{
    for (auto i : layout()->children) {
        if (i == view->layout()) {
            return;
        }
    }

    entering_views.push_back(view);
    view->parent = this;
    children.push_back(view);
    layout()->children.push_back(view->layout());
}

void view_t::remove_child(view_ptr view)
{
    exiting_views.push_back(view);

    view->parent = NULL;
    view_list::iterator it = children.begin();
    while (it != children.end()) {
        view_ptr v = *it;
        if (v == view) {
            children.erase(it);
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

view_ptr view_t::find_child(std::string uid)
{
    for (auto child : children) {
        if (child->uid == uid) {
            return child;
        }
    }
    return nullptr;
}

view_ptr view_t::find_child(view_type_e t)
{
    for (auto child : children) {
        if (child->is_type_of(t)) {
            return child;
        }
    }
    return nullptr;
}

view_ptr view_t::ptr()
{
    if (parent) {
        for (auto child : parent->children) {
            if (child.get() == this) {
                return child;
            }
        }
    }
    return nullptr;
}

void view_t::set_font(font_t* font)
{
    _font = font;
}

font_t* view_t::font()
{
    if (_font)
        return _font;

    // inherit from parent
    view_t* p = parent;
    while (p) {
        if (p->font()) {
            _font = p->font();
            return _font;
        }
        p = p->parent;
    }

    return system_t::instance()->renderer.default_font();
}

layout_item_ptr view_t::layout()
{
    if (!_layout) {
        _layout = std::make_shared<layout_item_t>();
        _layout->name = type_name();
    }
    return _layout;
}

void view_t::relayout()
{
    layout_run(layout(), {}, true);
}

void view_t::update()
{
    for (auto child : children) {
        child->update();
    }
}

void view_t::prerender()
{
    layout_item_ptr item = layout();
    state.pressed = false;
    state.hovered = is_hovered(this);
    state.rect = item->render_rect;
    state.scroll_x = item->scroll_x;
    state.scroll_y = item->scroll_y;
}

void view_t::render_frame(renderer_t* renderer)
{
    if (state.style.available) {
        render_styled_frame(renderer, layout()->render_rect, state.style);
        return;
    }

#if 1
    layout_item_ptr item = layout();

    color_t clr = { 255, 255, 255 };
    if (is_hovered(this)) {
        clr = { 255, 0, 255 };
    }

    rect_t r = item->render_rect;
    r.x += item->margin_left;
    r.y += item->margin_top;
    r.w -= (item->margin_left + item->margin_right);
    r.h -= (item->margin_top + item->margin_bottom);

    // renderer->draw_rect(r, {50,50,50}, true, 1.0f);
    renderer->draw_rect(r, clr, false, 1.0f);
#endif
}

void view_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
}

void view_t::propagate_event(event_t& event)
{
    dispatch_event(event);
    if (event.cancelled)
        return;

    if (parent) {
        parent->propagate_event(event);
    }
}

int view_t::state_hash(bool peek)
{
    int hash = murmur_hash(&state, sizeof(view_state_t), STATE_HASH_SEED);
    if (!peek) {
        _state_hash = hash;
    }
    return hash;
}

int view_t::content_hash(bool peek)
{
    if (!peek) {
        _content_hash = 0;
    }
    return 0;
}

void view_t::set_style(styled_frame_t style)
{
    state.style = style;
    state.style.available = true;
}

void view_t::rerender()
{
    _state_hash = 1;
    _content_hash = 4;
}

bool view_t::is_hovered(view_t* view)
{
    point_t p = { mouse_x, mouse_y };
    return view == view_hovered.get() && point_in_rect(p, view->layout()->render_rect);
}

bool view_t::set_hovered(view_t* view)
{
    if (view_hovered.get() != view) {
        system_t::instance()->caffeinate();
        if (view_hovered) {
            event_t event;
            event.type = EVT_HOVER_OUT;
            event.source = view_hovered.get();
            event.cancelled = false;
            view_hovered->propagate_event(event);
            view_hovered->rerender();
        }
        view_hovered = view->ptr();
        if (view_hovered) {
            event_t event;
            event.type = EVT_HOVER_IN;
            event.source = view_hovered.get();
            event.cancelled = false;
            view_hovered->propagate_event(event);
            view_hovered->rerender();
        }
    }
}

bool view_t::is_focused(view_t* view)
{
    return view == view_focused.get();
}

bool view_t::set_focused(view_t* view)
{
    if (view_focused.get() != view) {
        system_t::instance()->caffeinate();
        if (view_focused) {
            event_t event;
            event.type = EVT_FOCUS_OUT;
            event.source = view_focused.get();
            event.cancelled = false;
            view_focused->propagate_event(event);
            view_focused->rerender();
        }
        view_focused = view->ptr();
        if (view_focused) {
            event_t event;
            event.type = EVT_FOCUS_IN;
            event.source = view_focused.get();
            event.cancelled = false;
            view_focused->propagate_event(event);
            view_focused->rerender();
        }
    }
}

bool view_t::is_dragged(view_t* view)
{
    return view == view_dragged.get();
}

view_ptr _view_from_xy(view_ptr view, int x, int y)
{
    rect_t r = view->layout()->render_rect;

    point_t p = { x, y };
    if (point_in_rect(p, view->layout()->render_rect)) {

        // printf("%s %d %d %d %d\n", view->type_name().c_str(), r.x,r.y,r.w,r.h);
        // if (view->is_type_of(view_type_e::TEXT)) {
        //     printf("[%s]\n", view->cast<text_t>()->text().c_str());
        // }

        for (auto v : view->children) {
            view_ptr res = _view_from_xy(v, x, y);
            if (res && res->layout()->visible) {
                return res;
            }
        }

        return view;
    }
    return nullptr;
}

view_ptr view_from_xy(view_list& views, int x, int y)
{
    for (auto v : views) {
        view_ptr res = _view_from_xy(v, x, y);
        if (res) {
            return res;
        }
    }
    return nullptr;
}

bool view_offer_focus(view_ptr view)
{
    if (!view)
        return false;

    if (view->can_focus) {
        view_t::set_focused(view.get());
        return true;
    }
    if (view->parent) {
        return view_offer_focus(view->parent->ptr());
    }
    return false;
}

bool view_offer_hover(view_ptr view)
{
    if (!view)
        return false;

    if (view->can_hover) {
        view_t::set_hovered(view.get());
        return true;
    }
    if (view->parent) {
        return view_offer_hover(view->parent->ptr());
    }
    return false;
}

void view_dispatch_mouse_event(event_t& event, view_list& views)
{
    view_ptr view = view_from_xy(views, event.x, event.y);
    if (!view)
        return;

    event.source = view.get();

    switch (event.type) {
    case EVT_MOUSE_DOWN:
        view_pressed = view;

        view_offer_focus(view);

        view->propagate_event(event);
        if (!dragging) {
            drag_start_x = event.x;
            drag_start_y = event.y;
        }
        break;

    case EVT_MOUSE_UP:

        if (dragging && view_dragged) {
            event.source = view_dragged.get();
            event.type = EVT_MOUSE_DRAG_END;
            view_dragged->propagate_event(event);
            dragging = false;
            view_dragged = nullptr;
        }
        view_released = view;

        event.type = EVT_MOUSE_UP;
        event.cancelled = false;
        view->propagate_event(event);

        if (view_pressed == view_released) {
            event.type = EVT_MOUSE_CLICK;
            event.cancelled = false;
            view->propagate_event(event);
        }
        break;
    case EVT_MOUSE_MOTION:
        view->propagate_event(event);

        view_offer_hover(view);

        mouse_x = event.x;
        mouse_y = event.y;

        if (!dragging && view_pressed && event.button) {
            int dx = drag_start_x - event.x;
            int dy = drag_start_y - event.y;
            int drag_distance = dx * dx + dy * dy;
            if (drag_distance >= DRAG_THRESHOLD) {
                dragging = true;
                view_dragged = view_pressed;
                event.source = view_dragged.get();
                event.type = EVT_MOUSE_DRAG_START;
                event.cancelled = false;
                view_dragged->propagate_event(event);
            }
        }

        if (dragging && view_dragged) {
            event.source = view_dragged.get();
            event.type = EVT_MOUSE_DRAG;
            event.cancelled = false;
            view_dragged->propagate_event(event);
        }

        break;
    case EVT_MOUSE_WHEEL:
    default:
        view->propagate_event(event);
        break;
    }
}

void view_dispatch_other_event(event_t& event, view_list& views)
{
    for (auto v : views) {
        v->propagate_event(event);
    }
}

void view_dispatch_key_event(event_t& event, view_list& views)
{
    for (auto v : views) {
        if (view_t::is_focused(v.get())) {
            v->propagate_event(event);
        }
    }
}

void view_dispatch_events(event_list& events, view_list& views)
{
    view_list vl = views;

    if (popup_manager_t::instance()->children.size()) {
        vl = popup_manager_t::instance()->children;
    }

    for (auto& event : events) {
        switch (event.type) {
        case EVT_MOUSE_DOWN:
        case EVT_MOUSE_UP:
        case EVT_MOUSE_MOTION:
        case EVT_MOUSE_WHEEL:
            view_dispatch_mouse_event(event, vl);
            break;
        case EVT_KEY_TEXT:
        case EVT_KEY_SEQUENCE:
            view_dispatch_key_event(event, vl);
            break;
        default:
            view_dispatch_other_event(event, vl);
            break;
        }
    }
}

void view_propagate_stage_events(damage_t* damage)
{
    for (auto v : entering_views) {
        v->_state_hash = 0;
        event_t event;
        event.type = EVT_STAGE_IN;
        event.source = v.get();
        event.cancelled = false;
        v->propagate_event(event);
    }
    entering_views.clear();

    for (auto v : exiting_views) {
        if (damage) {
            damage->damage(v->layout()->render_rect);
        }
        event_t event;
        event.type = EVT_STAGE_OUT;
        event.source = v.get();
        event.cancelled = false;
        v->propagate_event(event);
    }
    exiting_views.clear();
}

void view_prerender(view_ptr view, view_list& visible_views, damage_t* damage)
{
    view_propagate_stage_events(damage);

    layout_item_ptr item = view->layout();
    if (!item->visible)
        return;

    if (item->render_rect.w <= 0 || item->render_rect.h <= 0)
        return;

    int prev_state_hash = view->_state_hash;
    int prev_content_hash = view->_content_hash;
    view_t::view_state_t prev_state = view->state;
    view->prerender();

    if (damage) {
        if (!rects_equal(prev_state.rect, view->state.rect)) {
            damage->damage(prev_state.rect);
        }
        if (!(prev_state_hash == view->state_hash() && prev_content_hash == view->content_hash())) {
            damage->damage(view->state.rect);
        }
    }

    visible_views.push_back(view);

    rect_t clip_rect = item->render_rect;
    if (view->parent && (view->parent->layout()->scroll_x || view->parent->layout()->scroll_y)) {
        clip_rect = view->parent->layout()->render_rect;
    }
    for (auto child : view->children) {
        if (rects_overlap(child->layout()->render_rect, clip_rect)) {
            view_prerender(child, visible_views, damage);
        }
    }
}

void view_render(renderer_t* renderer, view_ptr view, damage_t* damage)
{
    layout_item_ptr item = view->layout();
    if (!item->visible)
        return;

    if (damage && !damage->should_render(item->render_rect)) {
        return;
    }

    int pad = 0;
    rect_t clip = {
        item->render_rect.x - pad,
        item->render_rect.y - pad,
        item->render_rect.w + (pad * 2),
        item->render_rect.h + (pad * 2)
    };

    renderer->push_state();
    renderer->set_clip_rect(clip);

    view->render(renderer);

    bool has_deferred_rendering = false;
    for (auto child : view->children) {
        if (child->render_priority != 0) {
            has_deferred_rendering = true;
            continue;
        }
        view_render(renderer, child, damage);
    }

    if (has_deferred_rendering) {
        for (auto child : view->children) {
            if (child->render_priority > 0)
                view_render(renderer, child, damage);
        }
    }

    renderer->pop_state();
}
