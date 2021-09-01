#ifndef SCROLLAREA_VIEW_H
#define SCROLLAREA_VIEW_H

#include "view.h"

struct scrollarea_view : view_item {
    scrollarea_view();

	virtual bool mouse_wheel(int x, int y) override;

    view_item_ptr content;
    float move_factor_x;
    float move_factor_y;
};

#endif // SCROLLAREA_VIEW_H