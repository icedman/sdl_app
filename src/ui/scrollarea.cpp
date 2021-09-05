#include "scrollarea.h"
#include "renderer.h"

scrollarea_view::scrollarea_view()
    : view_item("scrollarea")
    , move_factor_x(20)
    , move_factor_y(20)
    , overscroll(0.05f)
{
    interactive = true;
    layout()->fit_children = false;
    content = std::make_shared<view_item>("content");
    content->layout()->fit_children = true;
    content->layout()->wrap = true;
    add_child(content);

    on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_wheel(evt.x, evt.y);
    });
}

bool scrollarea_view::mouse_wheel(int x, int y)
{
    if (view_item::mouse_wheel(x,y)) {
        return true;
    }
	layout()->scroll_x += x * move_factor_x;
    layout()->scroll_y += y * move_factor_y;
    // on_scroll();
	return true;
}
