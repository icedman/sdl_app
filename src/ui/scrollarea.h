#ifndef SCROLLAREA_VIEW_H
#define SCROLLAREA_VIEW_H

#include "view.h"

struct scrollarea_view : view_item {
    scrollarea_view();

	bool mouse_wheel(int y) override;

    view_item_ptr content;
};

#endif // SCROLLAREA_VIEW_H