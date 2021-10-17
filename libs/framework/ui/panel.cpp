#include "panel.h"
#include "damage.h"
#include "scrollarea.h"
#include "scrollbar.h"

panel_t::panel_t()
    : vertical_container_t()
{
    int scrollbar_size = 14;

    v_scroll = std::make_shared<vertical_scrollbar_t>();
    v_scroll->layout()->width = scrollbar_size;
    h_scroll = std::make_shared<horizontal_scrollbar_t>();
    h_scroll->layout()->height = scrollbar_size;

    scrollarea = std::make_shared<scrollarea_t>();

    view_ptr container = std::make_shared<horizontal_container_t>();
    container->layout()->order = 10;
    container->add_child(scrollarea);
    container->add_child(v_scroll);
    add_child(container);

    scrollarea->layout()->order = 10;
    v_scroll->layout()->order = 20;

    resizer = std::make_shared<view_t>();
    resizer->layout()->width = scrollbar_size;

    container = std::make_shared<horizontal_container_t>();
    container->layout()->height = scrollbar_size;
    container->add_child(h_scroll);
    container->add_child(resizer);
    add_child(container);

    bottom = container;
    bottom->layout()->order = 20;

    content()->layout()->wrap = false;
    content()->layout()->fit_children_x = true;
    content()->layout()->fit_children_y = true;

    v_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        evt.source = this->v_scroll.get();
        return this->handle_scrollbar_move(evt);
    });
    h_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        evt.source = this->h_scroll.get();
        return this->handle_scrollbar_move(evt);
    });

    on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
        if (evt.cancelled) {
            return true;
        }
        evt.cancelled = true;
        this->handle_mouse_wheel(evt);
        return true;
    });

    layout()->postlayout = [this](layout_item_t* item) {
        this->postlayout();
        return true;
    };
}

view_ptr panel_t::content()
{
    return scrollarea->cast<scrollarea_t>()->content();
}

void panel_t::_validate()
{
    scrollarea_t* area = scrollarea->cast<scrollarea_t>();
    scrollbar_t* vs = v_scroll->cast<scrollbar_t>();
    scrollbar_t* hs = h_scroll->cast<scrollbar_t>();

    int hp = (hs->count - hs->window);
    if (area->layout()->scroll_x < -hp) {
        area->layout()->scroll_x = -hp;
    }

    int vp = (vs->count - vs->window);
    if (area->layout()->scroll_y < -vp) {
        area->layout()->scroll_y = -vp;
    }

    if (area->layout()->scroll_x > 0) {
        area->layout()->scroll_x = 0;
    }
    if (area->layout()->scroll_y > 0) {
        area->layout()->scroll_y = 0;
    }

    layout_compute_absolute_position(layout());
}

bool panel_t::handle_scrollbar_move(event_t& event)
{
    scrollarea_t* area = scrollarea->cast<scrollarea_t>();
    scrollbar_t* vs = v_scroll->cast<scrollbar_t>();
    scrollbar_t* hs = h_scroll->cast<scrollbar_t>();

    if (vs->window < vs->count && (scrollbar_t*)event.source == vs) {
        int vp = ((float)(vs->count - vs->window) * vs->index / vs->count);
        area->layout()->scroll_y = -vp;
    }
    if (hs->window < hs->count && (scrollbar_t*)event.source == hs) {
        int hp = ((float)(hs->count - hs->window) * hs->index / hs->count);
        area->layout()->scroll_x = -hp;
    }

    _validate();
    return true;
}

bool panel_t::handle_mouse_wheel(event_t& event)
{
    scrollarea_t* area = scrollarea->cast<scrollarea_t>();
    scrollbar_t* vs = v_scroll->cast<scrollbar_t>();
    scrollbar_t* hs = h_scroll->cast<scrollbar_t>();

    area->layout()->scroll_x += event.sx * area->scroll_factor_x;
    area->layout()->scroll_y += event.sy * area->scroll_factor_y;

    _validate();

    int hi = 1.1f * -area->layout()->scroll_x;
    if (hi < 0)
        hi = 0;
    hs->set_index(hi);

    int vi = 1.1f * -area->layout()->scroll_y;
    if (vi < 0)
        vi = 0;
    vs->set_index(vi);
    return true;
}

void panel_t::postlayout()
{
    int ch = content()->layout()->rect.h;
    int cw = content()->layout()->rect.w;
    int sh = scrollarea->layout()->rect.h;
    int sw = scrollarea->layout()->rect.w;

    ((scrollbar_t*)v_scroll.get())->set_size(ch, sh);
    ((scrollbar_t*)h_scroll.get())->set_size(cw, sw);

    scrollbar_t* vs = v_scroll->cast<scrollbar_t>();
    scrollbar_t* hs = h_scroll->cast<scrollbar_t>();

    bool prev = bottom->layout()->visible;
    bottom->layout()->visible = hs->layout()->visible && !hs->disabled;
    if (hs->window >= hs->count) {
        bottom->layout()->visible = false;
    }

    if (prev != bottom->layout()->visible) {
        layout_request(layout());
    }
}

void panel_t::update_scrollbars()
{
    event_t event;
    event.x = 0;
    event.y = 0;
    handle_mouse_wheel(event);
}