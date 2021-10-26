#include "scrollbar.h"
#include "damage.h"
#include "renderer.h"

scrollbar_t::scrollbar_t()
    : scrollarea_t()
    , drag_offset(0)
    , dragging(false)
    , index(0)
    , window(6)
    , count(12)
{
    can_hover = true;

    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    content()->layout()->rgb = { 255, 255, 0 };

    on(EVT_MOUSE_CLICK, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_click(evt);
    });
    on(EVT_MOUSE_DRAG_START, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_drag_start(evt);
    });
    on(EVT_MOUSE_DRAG, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_drag(evt);
    });
    on(EVT_MOUSE_DRAG_END, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_drag_end(evt);
    });

    layout()->postlayout = [this](layout_item_t* item) {
        this->postlayout();
        return true;
    };
}

void scrollbar_t::postlayout()
{
    layout_item_ptr lo = layout();
    int th = (float)bar_size() * window / count;
    if (th < 40) {
        th = 40;
    }

    // set the thumb visual size
    if (lo->is_row()) {
        content()->layout()->width = th;
        content()->layout()->rect.w = th;
    } else {
        content()->layout()->height = th;
        content()->layout()->rect.h = th;
    }

    if (disabled) {
        lo->visible = false;
    }

    lo->state_hash = 0;
}

bool scrollbar_t::handle_mouse_drag_start(event_t& event)
{
    layout_item_ptr lo = layout();

    drag_offset = event.y - (lo->render_rect.y + lo->scroll_y);
    if (lo->is_row()) {
        drag_offset = event.x - (lo->render_rect.x + lo->scroll_x);
    }
    float th = thumb_size();
    if (drag_offset < 0 || drag_offset > th) {
        drag_offset = 0;
    }

    drag_offset = drag_offset - (th / 2);
    dragging = true;
    return true;
}

bool scrollbar_t::handle_mouse_drag_end(event_t& event)
{
    drag_offset = 0;
    dragging = false;
    return true;
}

bool scrollbar_t::handle_mouse_drag(event_t& event)
{
    _scroll(layout()->is_row() ? event.x : event.y);
    return true;
}

bool scrollbar_t::handle_mouse_click(event_t& event)
{
    _scroll(layout()->is_row() ? event.x : event.y);
    drag_offset = 0;
    dragging = false;
    return true;
}

void scrollbar_t::_scroll(int pos)
{
    layout_item_ptr lo = layout();

    float th = thumb_size();
    float pp = pos - (lo->is_row() ? lo->render_rect.x : lo->render_rect.y);
    float p = pp / bar_size();

    if (p < 0) {
        p = 0;
    }
    if (p > 1) {
        p = 1;
    }

    int newPos = bar_size() * p;
    newPos -= th / 2;
    newPos -= drag_offset;

    if (lo->is_row()) {
        lo->scroll_x = newPos;
    } else {
        lo->scroll_y = newPos;
    }

    _validate();
    layout_compute_absolute_position(layout());

    propagate_scrollbar_event();
}

void scrollbar_t::_validate()
{
    layout_item_ptr lo = layout();

    if (lo->scroll_y < 0) {
        lo->scroll_y = 0;
    }
    if (lo->scroll_y + content()->layout()->height > lo->render_rect.h) {
        lo->scroll_y = lo->render_rect.h - content()->layout()->height;
    }
    if (lo->scroll_x < 0) {
        lo->scroll_x = 0;
    }
    if (lo->scroll_x + content()->layout()->width > lo->render_rect.w) {
        lo->scroll_x = lo->render_rect.w - content()->layout()->width;
    }

    float th = thumb_size();
    float p = (float)scroll_pos() / (bar_size() - th);
    int idx = count * p;

    index = idx;
}

void scrollbar_t::propagate_scrollbar_event()
{
    _validate();
    event_t event;
    event.type = EVT_SCROLLBAR_MOVE;
    event.source = this;
    event.cancelled = false;
    propagate_event(event);
}

void scrollbar_t::set_index(int idx)
{
    if (idx == index) {
        return;
    }
    index = idx;
    float p = (float)idx / count;
    layout_item_ptr lo = layout();

    if (lo->is_row()) {
        lo->scroll_x = p * bar_size();
    } else {
        lo->scroll_y = p * bar_size();
    }

    _validate();
}

void scrollbar_t::set_size(int c, int w)
{
    if (c != count || w != window) {
        count = c;
        window = w;

        display = count * 0.95f > window;
        // layout()->visible = display;
        // layout()->visible = count > window;
        
        relayout();
    }
}

void scrollbar_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
    if (!display) return;

    layout_item_ptr lo = layout();
    layout_item_ptr lot = content()->layout();

    color_t clr;

    // clr = renderer->background;
    // clr.a = 50;
    // renderer->draw_rect(lo->render_rect, clr, true, 0);

    // thumb
    clr = renderer->foreground;
    clr.a = 50;
    if (is_hovered(this)) {
        clr.a = 100;
    }
    int pad = 4;
    renderer->draw_rect({ lot->render_rect.x + pad,
                            lot->render_rect.y + pad,
                            lot->render_rect.w - pad * 2,
                            lot->render_rect.h - pad * 2 },
        clr, true, 1, {}, 3);
}

int scrollbar_t::thumb_size()
{
    if (layout()->is_row()) {
        return content()->layout()->width;
    }
    return content()->layout()->height;
}

int scrollbar_t::bar_size()
{
    if (layout()->is_row()) {
        return layout()->render_rect.w;
    }
    return layout()->render_rect.h;
}

int scrollbar_t::scroll_pos()
{
    if (layout()->is_row()) {
        return layout()->scroll_x;
    }
    return layout()->scroll_y;
}
