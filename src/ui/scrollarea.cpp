#include "scrollarea.h"
#include "renderer.h"

scrollarea_view::scrollarea_view()
    : view_item("scrollarea")
{
    can_hover = true;
    can_scroll = true;
    content = std::make_shared<view_item>("content");
    content->layout()->fit_children = true;
    content->layout()->wrap = true;
    add_child(content);
}

bool scrollarea_view::mouse_wheel(int x, int y)
{
    int move = 20;
	layout()->scroll_x += x * move;
    layout()->scroll_y += y * move;
	// printf(">%s %d\n", type.c_str(), layout()->scroll_y);
	return true;
}
