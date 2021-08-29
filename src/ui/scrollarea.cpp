#include "scrollarea.h"
#include "renderer.h"

scrollarea_view::scrollarea_view()
    : view_item("scrollarea")
{
    content = std::make_shared<view_item>();
    content->layout()->fit_children = true;
    content->layout()->wrap = true;
    add_child(content);
}

bool scrollarea_view::mouse_wheel(int y)
{
	layout()->scroll_y += y * 4;
	printf(">%s %d\n", type.c_str(), layout()->scroll_y);
	return true;
}
