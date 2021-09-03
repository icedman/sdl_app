#include "scrollbar.h"
#include "renderer.h"
#include "render_cache.h"

scrollbar_view::scrollbar_view()
    : scrollarea_view()
    , drag_offset(0)
    , dragging(false)
    , index(0)
    , window(4)
    , count(12)
{
    interactive = true;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    type = "scrollbar";
    content->type = "thumb";
    content->layout()->rgb = { 255, 255, 0 };
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

    drag_offset = drag_offset - (th/2);
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
    newPos -= th/2;
    newPos -= drag_offset;
    
    if (lo->is_row()) {
        lo->scroll_x = newPos;
    } else {
        lo->scroll_y = newPos;
    }

    on_scroll();
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

bool scrollbar_view::on_scroll()
{
    _validate();
    if (parent) {
        ((view_item*)parent)->on_scroll();
    }
    return true;
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
}

void scrollbar_view::set_size(int c, int w)
{
    if (c != count || w != window) {
        count = c;
        window = w;
        layout_request();
    }
}

void scrollbar_view::render()
{
    // background
    layout_item_ptr lo = layout();
    draw_rect({
        lo->render_rect.x,
        lo->render_rect.y,
        lo->render_rect.w,
        lo->render_rect.h
    },
    { 255,0,255} , false, 1.0f);

    // thumb
    layout_item_ptr lot = content->layout();
    draw_rect({
        lot->render_rect.x + 4,
        lot->render_rect.y + 4,
        lot->render_rect.w - 8,
        lot->render_rect.h - 8
    },
    { 255,0,255}, false, (content->is_pressed() || is_hovered()) ? 2.0f : 1.0f);
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
