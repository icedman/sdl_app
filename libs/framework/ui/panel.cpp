#include "panel.h"
#include "damage.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "system.h"

// #define ENABLE_SENSITIVE_WHEEL

static inline bool same_sign(int num1, int num2)
{
    return num1 >= 0 && num2 >= 0 || num1 < 0 && num2 < 0;
}

panel_t::panel_t()
    : vertical_container_t()
    , wheel_x(0)
    , wheel_y(0)
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

    float hp = (hs->count - hs->window);
    if (area->layout()->scroll_x < -hp) {
        area->layout()->scroll_x = -hp;
    }

    float vp = (vs->count - vs->window);
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

#ifdef ENABLE_SENSITIVE_WHEEL
    if (event.x != -1 && event.y != -1) {
        if (!same_sign(wheel_x, event.sx))
            wheel_x = 0;
        if (!same_sign(wheel_y, event.sy))
            wheel_y = 0;
        wheel_x += wheel_x * 0.025f + event.sx * 40.0f;
        wheel_y += wheel_y * 0.025f + event.sy * 40.0f;
        if (wheel_y < -8000)
            wheel_y = -8000;
        if (wheel_y > 8000)
            wheel_y = 8000;
    }
#endif

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

    // can cause endless loops
    if (prev != bottom->layout()->visible) {
        relayout(1);
    }
}

void panel_t::update_scrollbars()
{
    event_t event;
    event.sx = 0;
    event.sy = 0;
    event.x = -1;
    event.y = -1;
    handle_mouse_wheel(event);
}

void panel_t::scroll_to_top()
{
    scrollarea->layout()->scroll_y = 0;
    update_scrollbars();
}

void panel_t::scroll_to_bottom()
{
    scrollarea->layout()->scroll_y = -content()->layout()->render_rect.h + scrollarea->layout()->render_rect.h;
    update_scrollbars();
}

void panel_t::update()
{
    if (wheel_y != 0) {
        wheel_y = (float)wheel_y * 0.99f;
        system_t::instance()->caffeinate();
        if (wheel_y > 20 || wheel_y < -20) {
            int coef = wheel_y / 50;
            if (coef == 0)
                coef = wheel_y < 0 ? -1 : 1;
            printf(">%d\n", coef);
            event_t event;
            event.sx = 0;
            event.sy = coef;
            event.x = -1;
            event.y = -1;
            handle_mouse_wheel(event);
            refresh();
        }
    }
    vertical_container_t::update();
}