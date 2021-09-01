#include "scrollarea.h"
#include "renderer.h"

scrollarea_view::scrollarea_view()
    : view_item("scrollarea")
    , move_factor_x(20)
    , move_factor_y(20)
{
    content = std::make_shared<view_item>("content");
    content->layout()->fit_children = true;
    content->layout()->wrap = true;
    add_child(content);
}

bool scrollarea_view::mouse_wheel(int x, int y)
{
	layout()->scroll_x += x * move_factor_x;
    layout()->scroll_y += y * move_factor_y;
    on_scroll();
	return true;
}
