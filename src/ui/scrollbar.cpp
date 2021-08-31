#include "scrollbar.h"
#include "renderer.h"

scrollbar_view::scrollbar_view()
    : scrollarea_view()
    , drag_offset_x(0)
    , drag_offset_y(0)
    , dragging(false)
    , index(0)
    , window(4)
    , count(12)
    , prevWindow(0)
    , prevCount(0)
{
    type = "scrollbar";
    can_press = true;
    can_drag = true;

    // content->can_scroll = true;
    // content->can_hover = true;
    content->type = "thumb";
    content->layout()->rgb = { 255, 255, 0 };
}

bool scrollbar_view::mouse_wheel(int x, int y)
{
    int move = 20;
    layout_item_ptr lo = layout();

    if (lo->is_row()) {
        lo->scroll_x += x * move;
    } else {
        lo->scroll_y += y * move;
    }

    on_scroll();
    return true;
}

bool scrollbar_view::mouse_drag_start(int x, int y)
{
    layout_item_ptr lo = layout();

    drag_offset_y = y - (lo->render_rect.y + lo->scroll_y);
    float th = content->layout()->height;
    if (drag_offset_y < 0 || drag_offset_y > th) {
        drag_offset_y = 0;
    }

    drag_offset_y = drag_offset_y - (th/2);
    dragging = true;
    return true;
}

bool scrollbar_view::mouse_drag_end(int x, int y)
{
    drag_offset_x = 0;
    drag_offset_y = 0;
    dragging = false;
    return true;
}

bool scrollbar_view::mouse_drag(int x, int y)
{
    mouse_click(x, y, 0);
    return true;
}

bool scrollbar_view::mouse_click(int x, int y, int button)
{
    _scroll(x, y);
    mouse_drag_end(x, y);
    return true;
}

void scrollbar_view::_scroll(int x, int y)
{
    layout_item_ptr lo = layout();

    float th = content->layout()->height;
    float yy = y - lo->render_rect.y;
    float p = yy / lo->render_rect.h;

    if (p < 0) {
        p = 0;
    }
    if (p > 1) {
        p = 1;
    }

    lo->scroll_y = lo->render_rect.h * p;
    lo->scroll_y -= th/2;
    lo->scroll_y -= drag_offset_y;

    on_scroll();
}

void scrollbar_view::precalculate()
{
    layout_item_ptr lo = layout();
    int th = (float)lo->render_rect.h * window / count;
    if (th < 40) {
        th = 40;
    }
    content->layout()->height = th;
    content->layout()->rect.h = content->layout()->height;
    content->layout()->render_rect.h = content->layout()->height;
}

bool scrollbar_view::on_scroll()
{
    layout_item_ptr lo = layout();

    if (prevCount != count || prevWindow != window) {
        prevCount = count;
        prevWindow = window;
        layout_request();
    }
    
    // printf("%f\n", (float)content->layout()->render_rect.h);

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

    float th = content->layout()->height;
    float p = (float)lo->scroll_y / (lo->render_rect.h - th);
    int idx = count * p;

    index = idx;
    if (parent) {
        ((view_item*)parent)->on_scroll();
    }
    return true;
}

void scrollbar_view::set_index(int idx)
{
    index = idx;
    float p = (float)idx / count;
    layout_item_ptr lo = layout();
    lo->scroll_y = p * lo->render_rect.h;
    on_scroll();
}

void scrollbar_view::set_size(int c, int w)
{
    count = c;
    window = w;
    on_scroll();
}