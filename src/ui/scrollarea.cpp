#include "scrollarea.h"
#include "renderer.h"

scrollarea_view::scrollarea_view()
    : view_item()
    , move_factor_x(32)
    , move_factor_y(32)
    , prev_scroll_x(-1)
    , prev_scroll_y(-1)
{
    interactive = true;
    layout()->fit_children = false;
    content = std::make_shared<view_item>();
    content->layout()->fit_children = true;
    content->layout()->wrap = true;
    add_child(content);

    on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
        if (evt.cancelled) {
            return true;
        }
        evt.cancelled = true;
        return this->mouse_wheel(evt.x, evt.y);
    });
}

bool scrollarea_view::mouse_wheel(int x, int y)
{
    if (Renderer::instance()->is_terminal()) {
        move_factor_x = 1;
        move_factor_y = 1;
    }

    layout()->scroll_x += x * move_factor_x;
    layout()->scroll_y += y * move_factor_y;
    return true;
}

void scrollarea_view::prerender()
{
    view_item::prerender();

    layout_item_ptr alo = layout();
    if (prev_scroll_x != alo->scroll_x || prev_scroll_y != alo->scroll_y) {
        damage();
        prev_scroll_x = alo->scroll_x;
        prev_scroll_y = alo->scroll_y;
    }
}