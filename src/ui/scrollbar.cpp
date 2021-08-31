#include "scrollbar.h"
#include "renderer.h"

scrollbar_view::scrollbar_view()
    : scrollarea_view()
    , drag_offset_x(0)
    , drag_offset_y(0)
    , index(0)
    , window(4)
    , count(12)
{
    type = "scrollbar";
    can_press = true;
    can_drag = true;

    // content->can_scroll = true;
    // content->can_hover = true;
    content->layout()->height = 270;
    content->layout()->rgb = { 255, 255, 0 };
}

bool scrollbar_view::mouse_wheel(int x, int y)
{
    int move = 20;
    layout_item_ptr l = layout();

    if (l->is_row()) {
        l->scroll_x += x * move;
    } else {
        l->scroll_y += y * move;
    }

    on_scroll();
    return true;
}

bool scrollbar_view::mouse_drag_start(int x, int y)
{
    layout_item_ptr l = layout();

    drag_offset_y = y - (l->render_rect.y + l->scroll_y);
    float th = content->layout()->height;
    if (drag_offset_y < 0 || drag_offset_y > th) {
        drag_offset_y = 0;
    }

    drag_offset_y = drag_offset_y - (th/2);
    return true;
}

bool scrollbar_view::mouse_drag_end(int x, int y)
{
    drag_offset_x = 0;
    drag_offset_y = 0;
    return true;
}

bool scrollbar_view::mouse_drag(int x, int y)
{
    mouse_click(x, y, 0);
    return true;
}

bool scrollbar_view::mouse_click(int x, int y, int button)
{
    layout_item_ptr l = layout();

    float th = content->layout()->height;
    float yy = y - l->render_rect.y;
    float p = yy / l->render_rect.h;

    if (p < 0) {
        p = 0;
    }
    if (p > 1) {
        p = 1;
    }

    l->scroll_y = l->render_rect.h * p;
    l->scroll_y -= th/2;
    l->scroll_y -= drag_offset_y;

    on_scroll();
    return true;
}

void scrollbar_view::on_scroll()
{
    layout_item_ptr l = layout();

    content->layout()->height = (float)l->render_rect.h * window / count;
    content->layout()->render_rect.h = content->layout()->height;
    
    // printf("%f\n", (float)content->layout()->render_rect.h);

    if (l->scroll_y < 0) {
        l->scroll_y = 0;
    }
    if (l->scroll_y + content->layout()->height > l->render_rect.h) {
        l->scroll_y = l->render_rect.h - content->layout()->height;
    }
    if (l->scroll_x < 0) {
        l->scroll_x = 0;
    }
    if (l->scroll_x + content->layout()->width > l->render_rect.w) {
        l->scroll_x = l->render_rect.w - content->layout()->width;
    }

    float th = content->layout()->height;
    float p = (float)l->scroll_y / (l->render_rect.h - th);
    int idx = count * p;

    // printf("%d %f\n", idx, p);
}