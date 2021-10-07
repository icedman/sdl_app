#include "splitter.h"

splitter_view::splitter_view()
    : view_item()
    , dragging(false)
{
    layout()->width = 4;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    class_name = "item";
    interactive = true;

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


bool splitter_view::mouse_drag_start(int x, int y)
{
    if (!left) return true;
    start_width = left->layout()->width;
    drag_start_x = x;
    drag_start_y = y;
    dragging = true;
    return true;
}

bool splitter_view::mouse_drag_end(int x, int y)
{
    dragging = false;
    return true;
}

bool splitter_view::mouse_drag(int x, int y)
{
    if (dragging) {
        left->layout()->width = start_width + x - drag_start_x;
        // layout_recompute(container->layout());
        // container->damage();
        layout_request();
        Renderer::instance()->throttle_up_events();
    }
    return true;
}

void splitter_view::prerender()
{
    std::string mod;
    if (is_hovered()) {
        mod = ":hovered";
    }
    class_name = "splitter" + mod;

    view_item::prerender();
}

void splitter_view::render()
{
    view_style_t vs = style;

    layout_item_ptr lo = layout();
    layout_rect r = lo->render_rect;

    RenColor clr = { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue, 255 };
    Renderer::instance()->draw_rect({ r.x, r.y, r.w, r.h }, clr, vs.filled, 0);
}