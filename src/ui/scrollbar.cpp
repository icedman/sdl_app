#include "scrollbar.h"
#include "renderer.h"

#include "app.h"
#include "style.h"

scrollbar_view::scrollbar_view()
    : scrollarea_view()
    , drag_offset(0)
    , dragging(false)
    , index(0)
    , window(12)
    , count(11)
{
    type = "scrollbar";

    interactive = true;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    content->type = "thumb";
    content->layout()->rgb = { 255, 255, 0 };

    on(EVT_MOUSE_CLICK, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_click(evt.x, evt.y, evt.button);
    });
    on(EVT_MOUSE_DRAG_START, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_drag_start(evt.x, evt.y);
    });
    on(EVT_MOUSE_DRAG, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_drag(evt.x, evt.y);
    });
    on(EVT_MOUSE_DRAG_END, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_drag_end(evt.x, evt.y);
    });
}

bool scrollbar_view::mouse_drag_start(int x, int y)
{
    layout_item_ptr lo = layout();

    drag_offset = y - (lo->render_rect.y + lo->scroll_y);
    if (lo->is_row()) {
        drag_offset = x - (lo->render_rect.x + lo->scroll_x);
    }
    float th = _thumb_size();
    if (drag_offset < 0 || drag_offset > th) {
        drag_offset = 0;
    }

    drag_offset = drag_offset - (th / 2);
    dragging = true;
    return true;
}

bool scrollbar_view::mouse_drag_end(int x, int y)
{
    drag_offset = 0;
    dragging = false;
    return true;
}

bool scrollbar_view::mouse_drag(int x, int y)
{
    _scroll(layout()->is_row() ? x : y);
    return true;
}

bool scrollbar_view::mouse_click(int x, int y, int button)
{
    _scroll(layout()->is_row() ? x : y);
    drag_offset = 0;
    dragging = false;
    return true;
}

void scrollbar_view::_scroll(int pos)
{
    layout_item_ptr lo = layout();

    float th = _thumb_size();
    float pp = pos - (lo->is_row() ? lo->render_rect.x : lo->render_rect.y);
    float p = pp / _bar_size();

    if (p < 0) {
        p = 0;
    }
    if (p > 1) {
        p = 1;
    }

    int newPos = _bar_size() * p;
    newPos -= th / 2;
    newPos -= drag_offset;

    if (lo->is_row()) {
        lo->scroll_x = newPos;
    } else {
        lo->scroll_y = newPos;
    }

    propagate_scrollbar_event();
}

void scrollbar_view::prelayout()
{
    layout_item_ptr lo = layout();
    int th = (float)_bar_size() * window / count;
    if (th < 40) {
        th = 40;
    }

    if (lo->is_row()) {
        content->layout()->width = th;
        content->layout()->rect.w = th;
    } else {
        content->layout()->height = th;
        content->layout()->rect.h = th;
    }

    if (disabled) lo->visible = false;
}

void scrollbar_view::postlayout()
{
    _validate();
}

void scrollbar_view::_validate()
{
    layout_item_ptr lo = layout();

    if (lo->scroll_y < 0) {
        lo->scroll_y = 0;
    }
    if (lo->scroll_y + content->layout()->height > lo->render_rect.h) {
        lo->scroll_y = lo->render_rect.h - content->layout()->height;
    }
    if (lo->scroll_x < 0) {
        lo->scroll_x = 0;
    }
    if (lo->scroll_x + content->layout()->width > lo->render_rect.w) {
        lo->scroll_x = lo->render_rect.w - content->layout()->width;
    }

    float th = _thumb_size();
    float p = (float)_scroll_pos() / (_bar_size() - th);
    int idx = count * p;

    index = idx;
}

void scrollbar_view::propagate_scrollbar_event()
{
    _validate();

    event_t event;
    event.type = EVT_SCROLLBAR_MOVE;
    event.source = this;
    event.cancelled = false;
    propagate_event(event);
}

void scrollbar_view::set_index(int idx)
{
    if (idx == index) {
        return;
    }
    index = idx;
    float p = (float)idx / count;
    layout_item_ptr lo = layout();

    if (lo->is_row()) {
        lo->scroll_x = p * _bar_size();
    } else {
        lo->scroll_y = p * _bar_size();
    }

    _validate();

    // printf("index %d %f\n", idx, p);
}

void scrollbar_view::set_size(int c, int w)
{
    if (c != count || w != window) {
        count = c;
        window = w;

        // todo compute rects

        layout()->visible = count > window;
        layout_request();
    }
}

void scrollbar_view::render()
{
    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("default");

    layout_item_ptr lo = layout();

    // background
    Renderer::instance()->draw_rect({ lo->render_rect.x,
                  lo->render_rect.y,
                  lo->render_rect.w,
                  lo->render_rect.h },
        { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue, 5 }, true);

    // thumb
    layout_item_ptr lot = content->layout();
    Renderer::instance()->draw_rect({ lot->render_rect.x + 4,
                  lot->render_rect.y + 4,
                  lot->render_rect.w - 8,
                  lot->render_rect.h - 8 },
        { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue, (content->is_pressed() || is_hovered()) ? 150 : 50 },
        true, 0, 3);
}

int scrollbar_view::_thumb_size()
{
    if (layout()->is_row()) {
        return content->layout()->width;
    }
    return content->layout()->height;
}

int scrollbar_view::_bar_size()
{
    if (layout()->is_row()) {
        return layout()->render_rect.w;
    }
    return layout()->render_rect.h;
}

int scrollbar_view::_scroll_pos()
{
    if (layout()->is_row()) {
        return layout()->scroll_x;
    }
    return layout()->scroll_y;
}
